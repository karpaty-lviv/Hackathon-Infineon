#include <project.h>

#include "car.h"
#include "music.h"
#include "cm4_common.h"

// ===============================================================================
// LINE FOLLOWING PID CONTROLLER CONFIGURATION
// ===============================================================================

// PID Gains - These values need tuning based on your robot's behavior
#define PID_KP          500.0    // Proportional gain: responds to current error
#define PID_KD          20    // Derivative gain: dampens oscillation
#define PID_KI          0.0     // Integral gain: eliminates steady-state error

double pidKp = PID_KP;
double pidKd = PID_KD;
double pidKi = PID_KI;

// Motor control parameters
#define BASE_SPEED      1000    // Base forward speed (range: -4000 to 4000)
#define MAX_CORRECTION  2000    // Maximum steering correction value

uint16_t baseSpeed = BASE_SPEED;
uint16_t maxCorrection = MAX_CORRECTION;



// PID state variables
static double lastError = 0.0;
static double integral = 0.0;
static uint32_t lastTime = 0;

// ===============================================================================
// HELPER FUNCTION: Calculate line position from 7 sensor binary reading
// ===============================================================================
// Returns: Position from -3000 (far left) to +3000 (far right), 0 = centered
// Input: 7-bit value where each bit represents one sensor (1 = line detected)
static double calculateLinePosition(uint8_t sensors)
{
    // CRITICAL: Sensor weights must match physical layout!
    // Assuming sensors are numbered left to right:
    // Sensor positions:  [0]   [1]   [2]   [3]   [4]   [5]   [6]
    //                   LEFT  LEFT  LEFT CENTER RIGHT RIGHT RIGHT
    // Weights (×1000):  -3000 -2000 -1000   0   +1000 +2000 +3000
    //
    // When line is to the LEFT:  negative position → turn LEFT
    // When line is to the RIGHT: positive position → turn RIGHT
    
    static const int16_t weights[7] = {-3000, -2000, -1000, 0, 1000, 2000, 3000};

    int32_t weightedSum = 0;
    int activeCount = 0;

    // Calculate weighted average of active sensors
    for (uint8_t i = 0; i < 7; i++)
    {
        if (sensors & (1 << i))  // Check if sensor i detected the line (1 = black line)
        {
            weightedSum += weights[i];
            activeCount++;
        }
    }

    // Return weighted average (position)
    // If no sensors detect line, return last known position
    if (activeCount > 0)
    {
        // FIXED: Use double division to avoid integer truncation!
        return (double)weightedSum / (double)activeCount;
    }
    else
    {
        // No line detected - use last error to guess direction
        // If robot was turning left (negative error), assume line is still left
        return (lastError < 0) ? -3000.0 : 3000.0;
    }
}


// ===============================================================================
// PID CONTROL FUNCTION
// ===============================================================================
// Calculates steering correction based on line position error
// Returns: Correction value to adjust motor speeds
//
// DETAILED EXPLANATION OF HOW THIS WORKS:
// ========================================
//
// PID stands for: Proportional-Integral-Derivative controller
// It's a feedback control loop that continuously calculates an error value
// and applies corrections to minimize that error.
//
// INPUT:  error = current line position (where line is relative to center)
//         currentTime = timestamp for calculating rate of change
//
// OUTPUT: correction = how much to adjust motor speeds (steering command)
//
// THE THREE TERMS:
// ----------------
//
// 1. PROPORTIONAL (P):
//    - pTerm = Kp × error
//    - Responds to CURRENT error
//    - Think: "How far off am I RIGHT NOW?"
//    - Larger Kp = more aggressive steering response
//    - If line is 1000 units right: pTerm = 40 × 1000 = 40000
//    - This makes right motor slow down, left motor speed up → turn right
//
// 2. DERIVATIVE (D):
//    - dTerm = Kd × (error - lastError) / deltaTime
//    - Responds to RATE OF CHANGE of error
//    - Think: "How fast am I approaching/leaving the line?"
//    - Acts like a brake - dampens oscillation
//    - If error was 1000, now it's 500 (decreasing):
//      → D term is negative → reduces correction (prevents overshoot)
//    - If error is increasing:
//      → D term is positive → increases correction (aggressive response)
//
// 3. INTEGRAL (I):
//    - iTerm = Ki × ∫(error × dt)
//    - Responds to ACCUMULATED error over time
//    - Think: "Have I been consistently off to one side?"
//    - Eliminates steady-state error (persistent offset)
//    - If robot always drifts slightly right:
//      → Integral builds up → adds permanent left correction
//
// ANTI-WINDUP:
//    - Limits integral to ±100 to prevent it from growing too large
//    - Without this, integral could accumulate during startup and cause huge overshoots
//
static int16_t pidControl(double error, uint32_t currentTime)
{
    // Calculate time difference in seconds
    double deltaTime = (currentTime - lastTime) / 1000.0;

    // Prevent division by zero on first iteration
    if (deltaTime < 0.001)
    {
        deltaTime = 0.001;
    }

    // ============================================================================
    // PROPORTIONAL TERM: Reacts to current error
    // ============================================================================
    // If error = +1000 (line is 1000 units to the right):
    //   pTerm = 40 × 1000 = 40000
    //   → This will slow down right motor, speed up left → turn right ✓
    //
    // If error = -1000 (line is 1000 units to the left):
    //   pTerm = 40 × (-1000) = -40000
    //   → This will slow down left motor, speed up right → turn left ✓
    
    double pTerm = pidKp * error;

    // ============================================================================
    // INTEGRAL TERM: Accumulates error over time
    // ============================================================================
    // Adds up error × time to catch persistent biases
    // Example: If robot consistently runs 100 units right:
    //   integral grows: 100 + 100 + 100 + ... = large value
    //   iTerm adds permanent left correction to compensate
    
    integral += error * deltaTime;

    // Anti-windup: Limit integral to prevent excessive accumulation
    // Without this limit, integral could grow huge during sharp turns
    if (integral > 100.0)  integral = 100.0;
    if (integral < -100.0) integral = -100.0;

    double iTerm = pidKi * integral;


    // ============================================================================
    // DERIVATIVE TERM: Reacts to rate of change
    // ============================================================================
    // Calculates how fast error is changing
    // If error is decreasing (we're correcting successfully):
    //   → Derivative is negative → reduces total correction (prevents overshoot)
    // If error is increasing (we're going more off-track):
    //   → Derivative is positive → increases total correction (aggressive fix)
    //
    // Example with Kd = 20:
    //   Previous error = 1000, current error = 500, deltaTime = 0.01s
    //   dTerm = 20 × (500 - 1000) / 0.01 = 20 × (-500) / 0.01 = -1,000,000
    //   → Large negative D-term reduces correction (we're getting closer, don't overshoot!)
    
    double dTerm = pidKd * (error - lastError) / deltaTime;

    // ============================================================================
    // COMBINE ALL THREE TERMS
    // ============================================================================
    // Total correction = P + I + D
    // P: reacts to current position
    // I: corrects for persistent bias
    // D: smooths the response and prevents oscillation
    
    double correction = pTerm + iTerm + dTerm;

    // Limit correction to prevent excessive steering
    // MAX_CORRECTION = 1500, so correction is clamped to [-1500, +1500]
    if (correction > maxCorrection)  correction = maxCorrection;
    if (correction < -maxCorrection) correction = -maxCorrection;

    // Update state variables for next iteration
    lastError = error;
    lastTime = currentTime;

    return (int16_t)correction;
}

// ===============================================================================
// LINE FOLLOWING FUNCTION
// ===============================================================================
// Reads sensors, calculates PID correction, and controls motors
static void followLine(void)
{
    // Read 7 track sensors (returns 7-bit value)
    uint8_t sensors = Track_Read();

    // Calculate line position: -3000 (left) to +3000 (right), 0 = centered
    double position = calculateLinePosition(sensors);

    // ============================================================================
    // CRITICAL: Error Calculation
    // ============================================================================
    // error = position - target
    //
    // Target is 0 (we want line centered)
    //
    // If position = +1000 (line is to the RIGHT):
    //   error = +1000 - 0 = +1000 (POSITIVE)
    //   → Need to turn RIGHT to center the line
    //   → Right motor slows down, left motor speeds up
    //
    // If position = -1000 (line is to the LEFT):
    //   error = -1000 - 0 = -1000 (NEGATIVE)
    //   → Need to turn LEFT to center the line
    //   → Left motor slows down, right motor speeds up
    
    double error = position - 0;  // Target position is 0 (center)

    // Get current time for PID calculation
    uint32_t currentTime = Timing_GetMillisecongs();

    // Calculate steering correction using PID
    int16_t correction = pidControl(error, currentTime);

    // ============================================================================
    // Apply differential steering:
    // ============================================================================
    //
    // LEFT MOTOR  = BASE_SPEED - correction
    // RIGHT MOTOR = BASE_SPEED + correction
    //
    // WHY THIS WAY:
    // If correction is POSITIVE (line is to the right, need to turn right):
    //   → Left motor:  BASE_SPEED - (+correction) = FASTER
    //   → Right motor: BASE_SPEED + (+correction) = SLOWER
    //   → Result: Robot turns RIGHT ✓
    //
    // If correction is NEGATIVE (line is to the left, need to turn left):
    //   → Left motor:  BASE_SPEED - (-correction) = SLOWER
    //   → Right motor: BASE_SPEED + (-correction) = FASTER
    //   → Result: Robot turns LEFT ✓


    int16_t leftSpeed = baseSpeed + correction;
    int16_t rightSpeed = baseSpeed - correction;

    // Constrain speeds to valid range
    if (leftSpeed > 4000)   leftSpeed = 4000;
    if (leftSpeed < -4000)  leftSpeed = -4000;
    if (rightSpeed > 4000)  rightSpeed = 4000;
    if (rightSpeed < -4000) rightSpeed = -4000;

    // Set motor speeds: Motor_Move(left_front, left_back, right_front, right_back)
    Motor_Move(-leftSpeed, -leftSpeed, -rightSpeed, -rightSpeed);
}

/* Implement ISR for I2C_1 */
void I2C_1_Isr(void)
{
    Cy_SCB_I2C_Interrupt(I2C_Main_HW, &I2C_Main_context);
}

/* Allocate buffer */
#define I2C_BUFFER_SIZE (128UL)
uint8_t i2C_buffer[I2C_BUFFER_SIZE];

ipc_msg_t ipcMsgForCM0 = {               /* IPC structure to be sent to CM0 */
    .clientId = IPC_CM4_TO_CM0_CLIENT_ID,
    .userCode = 0,
    .intrMask = CY_SYS_CYPIPE_INTR_MASK,
    .buffer   = {},
    .len      = 0
};

static void processIncomingIPCMessage(ipc_msg_t* msg);
static void processCM4Command(enum cm4CommandList cmd);

// Start flag
bool startCar = false;
bool motorsEnabled = false;

int main(void)
{
    /* SETUP */
    UART_START();
    DBG_PRINTF("Hello world!\r\n");

    // Initialize SCB for I2C operation, and configure desired data rate.
    // Hook I2C interrupt service routine and enable interrupt
    (void)Cy_SCB_I2C_Init(I2C_Main_HW, &I2C_Main_config, &I2C_Main_context);
    (void)Cy_SCB_I2C_SetDataRate(I2C_Main_HW, I2C_Main_DATA_RATE_HZ, I2C_Main_CLK_FREQ_HZ);
    Cy_SysInt_Init(&I2C_Main_SCB_IRQ_cfg, &I2C_1_Isr);
    NVIC_EnableIRQ(I2C_Main_SCB_IRQ_cfg.intrSrc);
    Cy_SCB_I2C_Enable(I2C_Main_HW);

    // Enable global interrupts.
    __enable_irq();

    /* Register the Message Callback */
    Cy_IPC_Pipe_RegisterCallback(CY_IPC_EP_CYPIPE_ADDR, CM4_MessageCallback, CY_IPC_EP_CYPIPE_CM0_ADDR);

    // Initialize SPI LED controller
    (void)Leds_Init();

    // Initialize driver that controls motors
    Motor_Init();

    // Initialize sound driver
    Sound_Init();

    // Initialize line tracking driver
    Track_Init();

    //Initialize timing driver
    Timing_Init();

    // Turn on LEDs on PSoC6 board
    Cy_GPIO_Clr(LEDG_0_PORT, LEDG_0_NUM); //green LED
    Cy_GPIO_Clr(LEDR_0_PORT, LEDR_0_NUM); //red LED

    // Music_FurElise();

    // Sound_Play(440u,1000u);

    // Demo movement sequence (removed for line following mode)
    /*
    Motor_Move(2000, 2000, 2000, 2000);     //go forward
    CyDelay(1000);
    Motor_Move(0, 0, 0, 0);                 //stop
    CyDelay(1000);
    Motor_Move(-1000, -1000, -1000, -1000); //go back
    CyDelay(2000);
    Motor_Move(0, 0, 0, 0);                 //stop
    CyDelay(1000);

    Motor_Move(-2000, -2000, 2000, 2000);   //turn left
    CyDelay(1000);
    Motor_Move(0, 0, 0, 0);                 //stop
    CyDelay(1000);
    Motor_Move(3000, 3000, -3000, -3000);   //turn right
    CyDelay(750);
    Motor_Move(0, 0, 0, 0);                 //stop
    CyDelay(1000);
    */

    uint32_t timeout = Timing_GetMillisecongs();
    uint32_t cycle = 0;

    // Initialize PID timer
    lastTime = Timing_GetMillisecongs();

    // Then execute remaining code
    Leds_FillSolidColor(0, 0, 0);

    for (;!startCar;) {
        if (CM4_isDataAvailableFromCM0()) {
            processIncomingIPCMessage(CM4_GetCM0Message());
        }
    }

    // MAIN LOOP
    for(;;)
    {
        // Check for new messages from CM0 core and process them. This is the most important task.
        if (CM4_isDataAvailableFromCM0()) {
            processIncomingIPCMessage(CM4_GetCM0Message());
        }
 
        // ========================================================================
        // LINE FOLLOWING - Execute PID control (only if motors enabled)
        // ========================================================================
        if (motorsEnabled)
        {
            followLine();
        }
        else
        {
            // Motors disabled - ensure they're stopped
            Motor_Move(0, 0, 0, 0);
        }


        // Blink Reg and blue LEDs based on timer
        //if((Timing_GetMillisecongs() - timeout) > 1000u)
        //{
        //    if (cycle == 0u)
        //    {
        //        Leds_PutPixel(7u, 0x00u, 0x55u, 0x00u);
        //        Leds_PutPixel(10u, 0x00u, 0x00u, 0x55u);
        //        cycle++;
        //    }
        //    else
        //    {
        //        Leds_PutPixel(7u, 0x00u, 0x00u, 0x55u);
        //        Leds_PutPixel(10u, 0x00u, 0x55u, 0x00u);
        //        cycle = 0;
        //    }
        //    timeout = Timing_GetMillisecongs();
        //}

        // Duplicate track sensor on Smart LEDs
        uint8_t track = Track_Read();
        for (uint8_t i=0; i<7u; i++)
        {
            Leds_PutPixel(i,track & 0x01u ? 0x55u : 0x00u, 0x00u, 0x00u);
            track = track >> 1;
        }


        Leds_Update();

        CyDelay(10);  // Reduced delay for faster PID loop (100Hz update rate)
    }
}

static void processIncomingIPCMessage(ipc_msg_t* msg)
{
    // In general, impossible situation, but never trust anyone.
    if (msg != NULL)
    {
        // Process messages with at least 1 byte of payload
        if (msg->len > 0)
        {
            // Decode message code. You would mostly like to use Commands
            switch (msg->userCode)
            {
                case IPC_USR_CODE_CMD:
                {
                    // Only [0] element was used for simplicity.
                    // IDEA: You could expand it to accept strings or more complex data packets!
                    // You can define array of strings and perform memcmp. This operation is a bit more costful.
                    processCM4Command((enum cm4CommandList)msg->buffer[0]);
                    break;
                }
                case IPC_USR_CODE_REQ:
                {
                    // Not implemented. You can be very creative here!
                    break;
                }
                default:
                    break;
            }
        }
    }
}

static void processCM4Command(enum cm4CommandList cmd)
{
    switch (cmd)
    {
        case CM4_COMMAND_START_CAR:
        {
            startCar = true;
            motorsEnabled = true;
            break;
        }
        case CM4_COMMAND_STOP_CAR:
        {
            motorsEnabled = false;
            Motor_Move(0, 0, 0, 0);
            break;
        }
        case CM4_COMMAND_ECHO:
        {
            ipc_msg_t* msg = CM4_GetCM0Message();

            if (msg->len >= 4)
            {
                uint8_t command = msg->buffer[1];
                uint16_t rawValue = (uint16_t)(msg->buffer[2]) | ((uint16_t)(msg->buffer[3]) << 8);
                int16_t value = (int16_t)rawValue;

                switch (command)
                {
                    case 0:
                        baseSpeed = value;
                        break;
                    case 1:
                        maxCorrection = value;
                        break;
                    case 2:
                        pidKp = (double)value;
                        break;
                    case 3:
                        pidKd = (double)value;
                        break;
                    case 4:
                        pidKi = (double)value;
                        break;
                }
            }
            break;
        }
        default:
            break;
    }
}

/* [] END OF FILE */
