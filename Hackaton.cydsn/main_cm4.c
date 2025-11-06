#include <project.h>

#include "car.h"
#include "music.h"
#include "cm4_common.h"

// ===============================================================================
// LINE FOLLOWING PID CONTROLLER CONFIGURATION
// ===============================================================================

// PID Gains - These values need tuning based on your robot's behavior
#define PID_KP          40.0    // Proportional gain: responds to current error
#define PID_KD          20.0    // Derivative gain: dampens oscillation
#define PID_KI          0.5     // Integral gain: eliminates steady-state error

// Motor control parameters
#define BASE_SPEED      1500    // Base forward speed (range: -4000 to 4000)
#define MAX_CORRECTION  1500    // Maximum steering correction value

// PID state variables
static double lastError = 0.0;
static double integral = 0.0;
static uint32_t lastTime = 0;

// ===============================================================================
// HELPER FUNCTION: Calculate line position from 7 sensor binary reading
// ===============================================================================
// Returns: Position from -3 (far left) to +3 (far right), 0 = centered
// Input: 7-bit value where each bit represents one sensor (1 = line detected)
static int calculateLinePosition(uint8_t sensors)
{
    // Sensor weight array: left sensors are negative, right sensors are positive
    // Sensor positions:  [0]  [1]  [2]  [3]  [4]  [5]  [6]
    // Weights:           -3   -2   -1    0   +1   +2   +3
    static const int8_t weights[7] = {-3, -2, -1, 0, 1, 2, 3};

    int weightedSum = 0;
    int activeCount = 0;

    // Calculate weighted average of active sensors
    for (uint8_t i = 0; i < 7; i++)
    {
        if (sensors & (1 << i))  // Check if sensor i detected the line
        {
            weightedSum += weights[i];
            activeCount++;
        }
    }

    // Return weighted average (position)
    // If no sensors detect line, return last known position
    if (activeCount > 0)
    {
        return weightedSum / activeCount;
    }
    else
    {
        // No line detected - use last error to guess direction
        return (lastError > 0) ? 3 : -3;
    }
}

// ===============================================================================
// PID CONTROL FUNCTION
// ===============================================================================
// Calculates steering correction based on line position error
// Returns: Correction value to adjust motor speeds
static int16_t pidControl(int error, uint32_t currentTime)
{
    // Calculate time difference in seconds
    double deltaTime = (currentTime - lastTime) / 1000.0;

    // Prevent division by zero on first iteration
    if (deltaTime < 0.001)
    {
        deltaTime = 0.001;
    }

    // PID FORMULA IMPLEMENTATION:
    // ---------------------------

    // P-term: Proportional to current error
    // Larger error = stronger correction
    double pTerm = PID_KP * error;

    // I-term: Integral of error over time
    // Accumulates past errors to eliminate steady-state offset
    integral += error * deltaTime;

    // Anti-windup: Limit integral to prevent excessive accumulation
    if (integral > 100.0)  integral = 100.0;
    if (integral < -100.0) integral = -100.0;

    double iTerm = PID_KI * integral;

    // D-term: Derivative (rate of change) of error
    // Predicts future error and dampens oscillation
    double dTerm = PID_KD * (error - lastError) / deltaTime;

    // Calculate total correction
    double correction = pTerm + iTerm + dTerm;

    // Limit correction to prevent excessive steering
    if (correction > MAX_CORRECTION)  correction = MAX_CORRECTION;
    if (correction < -MAX_CORRECTION) correction = -MAX_CORRECTION;

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

    // Calculate line position: -3 (left) to +3 (right), 0 = centered
    int position = calculateLinePosition(sensors);

    // Error is the deviation from center (target = 0)
    // Positive error = line is to the right
    // Negative error = line is to the left
    int error = 0 - position;  // Target position is 0 (center)

    // Get current time for PID calculation
    uint32_t currentTime = Timing_GetMillisecongs();

    // Calculate steering correction using PID
    int16_t correction = pidControl(error, currentTime);

    // Apply differential steering:
    // - If line is to the RIGHT (negative error, positive correction):
    //   Speed up left motors, slow down right motors (turn right)
    // - If line is to the LEFT (positive error, negative correction):
    //   Speed up right motors, slow down left motors (turn left)

    int16_t leftSpeed = BASE_SPEED + correction;
    int16_t rightSpeed = BASE_SPEED - correction;

    // Constrain speeds to valid range
    if (leftSpeed > 4000)   leftSpeed = 4000;
    if (leftSpeed < -4000)  leftSpeed = -4000;
    if (rightSpeed > 4000)  rightSpeed = 4000;
    if (rightSpeed < -4000) rightSpeed = -4000;

    // Set motor speeds: Motor_Move(left_front, left_back, right_front, right_back)
    Motor_Move(leftSpeed, leftSpeed, rightSpeed, rightSpeed);
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
            if (CM4_IsCM0Ready())
            {
                ipcMsgForCM0.userCode = IPC_USR_CODE_CMD;
                ipcMsgForCM0.len = CM4_GetCM0Message()->len;
                ipcMsgForCM0.buffer[0] = (uint8_t)CM0_SHARED_BLE_NTF_RELAY;
                memcpy(&ipcMsgForCM0.buffer[1], &CM4_GetCM0Message()->buffer[1], ipcMsgForCM0.len - 1);
                CM4_SendCM0Message(&ipcMsgForCM0);
            }
            break;
        }
        default:
            break;
    }
}

/* [] END OF FILE */
