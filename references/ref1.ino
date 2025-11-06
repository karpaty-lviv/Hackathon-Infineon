/*
 * ===============================================================================
 * LINE FOLLOWING ROBOT WITH DUAL PID CONTROL
 * ===============================================================================
 * 
 * GENERAL IDEA:
 * This program controls a line-following robot using two separate PID controllers:
 * 
 * 1. PRIMARY PID - Line Following Control:
 *    - Uses 7 IR sensors (QTR sensor array) to detect the line position
 *    - Calculates error as the deviation from center position (target = 3000 for 7 sensors)
 *    - PID formula adjusts the speed difference between left and right motors
 *    - Output: Steering correction to keep robot centered on the line
 * 
 * 2. SECONDARY PID - Motor Speed Synchronization:
 *    - Uses encoder feedback from both motors to measure actual RPM
 *    - Ensures both motors achieve their target speeds accurately
 *    - Compensates for motor differences and load variations
 *    - Output: PWM corrections for each motor individually
 * 
 * The system works by:
 *    - Reading line position from sensors
 *    - Calculating steering correction (Primary PID)
 *    - Setting target speeds for each motor (base_speed ± correction)
 *    - Measuring actual motor speeds via encoders
 *    - Fine-tuning PWM for each motor (Secondary PID)
 * 
 * ===============================================================================
 */

#include <QTRSensors.h>
#include <SoftwareSerial.h>

// ===============================================================================
// SENSOR CONFIGURATION (7 SENSORS)
// ===============================================================================
QTRSensors qtr;
const uint8_t SensorCount = 7;  // Changed from 6 to 7 sensors
uint16_t sensorValues[SensorCount];

// Target position for 7 sensors: 0 (far left) to 6000 (far right)
// Center position is 3000 (middle of sensor array)
int target = 3000;  // Changed from 2500 to 3000 for 7-sensor array

// ===============================================================================
// PRIMARY PID CONTROLLER - LINE FOLLOWING (Sensor-based)
// ===============================================================================
// Tuned for max_speed = 80
// Critical gain testing: K_critical = 0.03, frequency = 1.507 Hz
int max_speed = 80;

// PID GAINS for line following:
double Kp = 0.02178;  // Proportional gain (original: 0.018, optimized: 0.02178)
double Kd = 3.014;    // Derivative gain (prevents overshoot and oscillation)
double Ki = 0.083;    // Integral gain (eliminates steady-state error)

// PID state variables for line following
double lastError = 0;           // Previous error for derivative calculation
double integral_Error = 0;      // Accumulated error for integral calculation
double correction;              // PID output: steering correction

// ===============================================================================
// MOTOR CONTROL PINS
// ===============================================================================
int motorRIGHT_PWM = 10;   // PWM control for right motor speed
int motorLEFT_PWM = 6;     // PWM control for left motor speed
int motorRIGHTfram = 9;    // Right motor forward direction pin
int motorRIGHTbak = 4;     // Right motor backward direction pin
int motorLEFTfram = 5;     // Left motor forward direction pin
int motorLEFTbak = 7;      // Left motor backward direction pin

// ===============================================================================
// ENCODER PINS AND VARIABLES
// ===============================================================================
int interruptPin_LEFT = 2;   // Encoder interrupt pin for left motor
int interruptPin_RIGHT = 3;  // Encoder interrupt pin for right motor

// Encoder counters (volatile because modified in interrupt routines)
volatile int encoder_LEFTcount = 0;   // Left motor encoder pulse count
volatile int encoder_RIGHTcount = 0;  // Right motor encoder pulse count

// ===============================================================================
// SECONDARY PID CONTROLLER - MOTOR SPEED SYNCHRONIZATION (Encoder-based)
// ===============================================================================
// PID GAINS for motor speed control (deadtime tuned):
double Kpr = 0.015;    // Proportional gain (K_max = 0.025)
double Kdr = 1.54045;  // Derivative gain (frequency = 0.770246 Hz)
double Kir = 0.0;      // Integral gain (set to 0, running without integral term)

// PID state variables for right motor
double Error_encoderRIGHT = 0;
double integral_Error_encoderRIGHT = 0;
double lastError_encoderRIGHT = 0;
double correction_RIGHT;  // PWM correction for right motor

// PID state variables for left motor
double Error_encoderLEFT = 0;
double integral_Error_encoderLEFT = 0;
double lastError_encoderLEFT = 0;
double correction_LEFT;   // PWM correction for left motor

// Encoder timing and counting variables
unsigned long encoder_lastime = 0;
volatile int encoder_last_RIGHTcount = 0;
volatile int encoder_last_LEFTcount = 0;

// ===============================================================================
// TIMING VARIABLES
// ===============================================================================
unsigned long last_time = 0;    // Last loop execution time for primary PID
unsigned long current_time;     // Current time
unsigned long delta_T;          // Time difference for derivative/integral calculations

// ===============================================================================
// TESTING AND DEBUG VARIABLES
// ===============================================================================
int TestCounter = 1;
double test_error = 0;               // Accumulated absolute error for testing
double last_testTIME = 0;
double testTIME_intervalls = 3000;   // Print test results every 3 seconds

// ===============================================================================
// SETUP FUNCTION
// ===============================================================================
void setup()
{
    // Configure motor control pins as outputs
    pinMode(motorLEFTfram, OUTPUT);
    pinMode(motorLEFT_PWM, OUTPUT);
    pinMode(motorLEFTbak, OUTPUT);
    pinMode(motorRIGHTfram, OUTPUT);
    pinMode(motorRIGHT_PWM, OUTPUT);
    pinMode(motorRIGHTbak, OUTPUT);
    
    // Configure encoder pins as inputs with pull-up resistors
    pinMode(interruptPin_LEFT, INPUT_PULLUP);
    pinMode(interruptPin_RIGHT, INPUT_PULLUP);  // Fixed: was interruptPin_LEFT twice
    
    // Initialize serial communication for debugging
    Serial.begin(9600);
    
    // Attach interrupt handlers to count encoder pulses
    // CHANGE mode triggers on both rising and falling edges
    attachInterrupt(digitalPinToInterrupt(interruptPin_LEFT), counter_LEFT, CHANGE);
    attachInterrupt(digitalPinToInterrupt(interruptPin_RIGHT), counter_RIGHT, CHANGE);
    
    // 7 sensors connected to A0 through A6
    qtr.setSensorPins((const uint8_t[]){A0, A1, A2, A3, A4, A5, A6}, SensorCount);
    
    // ===============================================================================
    // SENSOR CALIBRATION ROUTINE
    // ===============================================================================
    // Wait 5 seconds before starting calibration (time to position robot)
    delay(5000);
    
    // Turn on built-in LED to indicate calibration mode
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);
    
    // Calibration by rotating robot back and forth over the line
    // This maps the min/max sensor values for better line detection
    // Each analogRead() takes ~0.1 ms
    // 0.1 ms × 4 samples × 7 sensors × 10 reads = ~28 ms per calibrate() call
    
    // Start motors at calibration speed
    analogWrite(motorRIGHT_PWM, 90);
    analogWrite(motorLEFT_PWM, 90);
    
    // Define encoder positions for oscillation during calibration
    int myRotational_position[] = {100, 300, 500, 700, 900, 1100, 1300, 1500, 1700, 1800};
    
    // Perform 5 oscillations (counterclockwise then clockwise)
    for (int i = 0; i < 5; i++)
    {
        // COUNTERCLOCKWISE ROTATION (right motor forward, left motor backward)
        digitalWrite(motorRIGHTfram, HIGH);
        digitalWrite(motorRIGHTbak, LOW);
        digitalWrite(motorLEFTfram, LOW);
        digitalWrite(motorLEFTbak, HIGH);
        
        // Rotate until target position reached, calibrating sensors continuously
        while (encoder_RIGHTcount < myRotational_position[2 * i])
        {
            qtr.calibrate();
        }
        
        // Stop motors
        digitalWrite(motorRIGHTfram, LOW);
        digitalWrite(motorRIGHTbak, LOW);
        digitalWrite(motorLEFTfram, LOW);
        digitalWrite(motorLEFTbak, LOW);
        delay(2000);  // Pause between rotations
        
        // CLOCKWISE ROTATION (right motor backward, left motor forward)
        digitalWrite(motorRIGHTfram, LOW);
        digitalWrite(motorRIGHTbak, HIGH);
        digitalWrite(motorLEFTfram, HIGH);
        digitalWrite(motorLEFTbak, LOW);
        
        // Rotate until target position reached, calibrating sensors continuously
        while (encoder_RIGHTcount < myRotational_position[2 * i + 1])
        {
            qtr.calibrate();
        }
        
        // Stop motors
        digitalWrite(motorRIGHTfram, LOW);
        digitalWrite(motorRIGHTbak, LOW);
        digitalWrite(motorLEFTfram, LOW);
        digitalWrite(motorLEFTbak, LOW);
        delay(2000);  // Pause between rotations
    }
    
    // Turn off LED to indicate calibration complete
    digitalWrite(LED_BUILTIN, LOW);
    
    // Wait 3 seconds before starting main loop
    delay(3000);
}

// ===============================================================================
// MAIN LOOP - LINE FOLLOWING WITH DUAL PID CONTROL
// ===============================================================================
void loop()
{
    // ===============================================================================
    // STEP 1: TIME MANAGEMENT
    // ===============================================================================
    current_time = millis();                      // Get current time in milliseconds
    delta_T = (current_time - last_time) / 1000;  // Calculate time difference in seconds
    
    // ===============================================================================
    // STEP 2: READ SENSOR ARRAY AND CALCULATE LINE POSITION
    // ===============================================================================
    // Read calibrated sensor values and get line position
    // For 7 sensors: position ranges from 0 (far left) to 6000 (far right)
    // Center position is 3000
    // Returns weighted average of sensor readings (black line on white surface)
    uint16_t position = qtr.readLineBlack(sensorValues);
    
    // ===============================================================================
    // STEP 3: PRIMARY PID CONTROLLER - LINE FOLLOWING
    // ===============================================================================
    // Calculate error: how far the line is from center
    // Negative error = line is to the left (position < 3000)
    // Positive error = line is to the right (position > 3000)
    int Error = target - position;
    
    // *** PRIMARY PID FORMULA (Line Following) ***
    // Base case for first loop iteration (avoid division by very small delta_T)
    if (delta_T < 0.00007)
    {
        // Use only proportional term on first iteration
        correction = Kp * Error;
    }
    else
    {
        // FULL PID FORMULA:
        // correction = P_term + D_term + I_term
        // 
        // P_term = Kp * Error
        //   - Proportional: responds to current error
        //   - Larger error = larger correction
        // 
        // D_term = (Kd / delta_T) * (Error - lastError)
        //   - Derivative: responds to rate of change
        //   - Prevents overshoot and reduces oscillation
        //   - If error is decreasing, this reduces correction
        // 
        // I_term = delta_T * Ki * integral_Error
        //   - Integral: responds to accumulated past errors
        //   - Eliminates steady-state error
        //   - Helps when robot consistently drifts to one side
        
        correction = Kp * Error + 
                     ((Kd / delta_T) * (Error - lastError)) + 
                     (delta_T * Ki * integral_Error);
    }
    
    // ===============================================================================
    // STEP 4: CALCULATE TARGET RPM FOR EACH MOTOR
    // ===============================================================================
    // Apply steering correction to base speed:
    // - If line is to the RIGHT (positive error): slow down right motor, speed up left
    // - If line is to the LEFT (negative error): slow down left motor, speed up right
    double Wanted_RPM_LEFT = RPM_LEFT_calculator(max_speed + correction);
    double Wanted_RPM_RIGHT = RPM_RIGHT_calculator(max_speed - correction);
    
    // ===============================================================================
    // STEP 5: READ ENCODER COUNTS
    // ===============================================================================
    // Capture current encoder counts (volatile variables updated by interrupts)
    unsigned encoder_current_LEFTcount = encoder_LEFTcount;
    unsigned encoder_current_RIGHTcount = encoder_RIGHTcount;
    
    // ===============================================================================
    // STEP 6: SECONDARY PID CONTROLLER - MOTOR SPEED SYNCHRONIZATION
    // ===============================================================================
    // Execute every 50 milliseconds to maintain consistent RPM calculations
    if (current_time - encoder_lastime >= 50)
    {
        // Calculate time interval for this encoder update
        double delta_T_encoder = current_time - encoder_lastime;
        
        // Calculate number of encoder pulses since last check
        // Note: Each encoder pulse corresponds to two state changes
        double number_of_pulsRIGHT = encoder_current_RIGHTcount - encoder_last_RIGHTcount;
        double number_of_pulsLEFT = encoder_current_LEFTcount - encoder_last_LEFTcount;
        
        // Calculate actual motor RPM from encoder pulses
        // Formula: RPM = (pulses / pulses_per_revolution) * (60 seconds / time_in_seconds)
        // Assuming 24 pulses per revolution (12 encoder slots × 2 edges)
        double RPM_RIGHT = 60 * (number_of_pulsRIGHT / 24) / ((delta_T_encoder) / 1000);
        double RPM_LEFT = 60 * (number_of_pulsLEFT / 24) / ((delta_T_encoder) / 1000);
        
        // Calculate RPM error for each motor
        // Positive error = motor is running slower than desired
        // Negative error = motor is running faster than desired
        double Error_encoderRIGHT = Wanted_RPM_RIGHT - RPM_RIGHT;
        double Error_encoderLEFT = Wanted_RPM_LEFT - RPM_LEFT;
        
        // Optional debugging output (commented out for performance)
        // Serial.print(Wanted_RPM_LEFT);
        // Serial.print(" -Wanted- ");
        // Serial.print(Wanted_RPM_RIGHT);
        // Serial.println();
        // Serial.print(RPM_LEFT);
        // Serial.print(" -Actual- ");
        // Serial.print(RPM_RIGHT);
        // Serial.println();
        // Serial.print(Error_encoderLEFT);
        // Serial.print(" - Encoder Errors - ");
        // Serial.print(Error_encoderRIGHT);
        // Serial.println();
        
        // *** SECONDARY PID FORMULA (Right Motor Speed Control) ***
        // correction_RIGHT = P_term + D_term + I_term
        // 
        // P_term = Kpr * Error_encoderRIGHT
        // D_term = (Kdr / delta_T_encoder) * (Error_encoderRIGHT - lastError_encoderRIGHT)
        // I_term = delta_T_encoder * Kir * integral_Error_encoderRIGHT
        // 
        // This correction is ADDED to the PWM to compensate for motor speed errors
        correction_RIGHT = Kpr * Error_encoderRIGHT +
                          ((Kdr / delta_T_encoder) * (Error_encoderRIGHT - lastError_encoderRIGHT)) +
                          (delta_T_encoder * Kir * integral_Error_encoderRIGHT);
        
        // *** SECONDARY PID FORMULA (Left Motor Speed Control) ***
        // Same PID formula applied to left motor
        correction_LEFT = Kpr * Error_encoderLEFT +
                         ((Kdr / delta_T_encoder) * (Error_encoderLEFT - lastError_encoderLEFT)) +
                         (delta_T_encoder * Kir * integral_Error_encoderLEFT);
        
        // Update integral terms (accumulated error over time)
        integral_Error_encoderRIGHT += Error_encoderRIGHT;
        integral_Error_encoderLEFT += Error_encoderLEFT;
        
        // Store current errors for next derivative calculation
        lastError_encoderRIGHT = Error_encoderRIGHT;
        lastError_encoderLEFT = Error_encoderLEFT;
        
        // Update encoder reference counts for next iteration
        encoder_last_RIGHTcount = encoder_current_RIGHTcount;
        encoder_last_LEFTcount = encoder_current_LEFTcount;
        
        // Update timestamp
        encoder_lastime = current_time;
    }
    
    // ===============================================================================
    // STEP 7: APPLY COMBINED CORRECTIONS TO MOTORS
    // ===============================================================================
    // Calculate final PWM values by combining:
    // 1. Base speed (max_speed)
    // 2. Primary PID steering correction (±correction)
    // 3. Secondary PID speed synchronization (correction_LEFT/RIGHT)
    //
    // LEFT MOTOR = base_speed + steering_correction + speed_correction
    // RIGHT MOTOR = base_speed - steering_correction + speed_correction
    //
    // constrain() limits PWM to valid range (0-255)
    
    analogWrite(motorLEFT_PWM, constrain(max_speed + correction + correction_LEFT, 0, 255));
    analogWrite(motorRIGHT_PWM, constrain(max_speed - correction + correction_RIGHT, 0, 255));
    
    // Set motor directions to forward
    digitalWrite(motorLEFTfram, HIGH);
    digitalWrite(motorLEFTbak, LOW);
    digitalWrite(motorRIGHTfram, HIGH);
    digitalWrite(motorRIGHTbak, LOW);
    
    // ===============================================================================
    // STEP 8: UPDATE PRIMARY PID STATE VARIABLES
    // ===============================================================================
    // Store current error for next derivative calculation
    lastError = Error;
    
    // Accumulate error for integral term
    integral_Error += Error;
    
    // Update timestamp for next loop iteration
    last_time = current_time;
    
    // ===============================================================================
    // STEP 9: TESTING AND PERFORMANCE MONITORING
    // ===============================================================================
    // Accumulate absolute error for performance evaluation
    test_error += abs(Error);
    
    // Print accumulated error every 3 seconds
    if (current_time - last_testTIME >= testTIME_intervalls)
    {
        Serial.print("Test ");
        Serial.print(TestCounter);
        Serial.print(": Accumulated Error = ");
        Serial.print(test_error);
        Serial.println();
        
        last_testTIME = current_time;
        TestCounter++;
    }
    
    // Small delay to prevent loop from running too fast
    delay(40);
}

// ===============================================================================
// INTERRUPT SERVICE ROUTINES (ISR)
// ===============================================================================
// These functions are called automatically when encoder signals change
// They increment the encoder counters to track motor rotation

void counter_LEFT()
{
    encoder_LEFTcount++;
}

void counter_RIGHT()
{
    encoder_RIGHTcount++;
}

// ===============================================================================
// HELPER FUNCTIONS - RPM CALCULATION FROM PWM
// ===============================================================================
// These functions convert desired PWM values to expected RPM
// The relationship depends on motor characteristics and should be calibrated

double RPM_LEFT_calculator(double pwm_value)
{
    // TODO: Implement actual PWM to RPM conversion for left motor
    // This should be based on motor characterization data
    // Example: return (pwm_value * motor_constant + offset);
    return pwm_value * 1.0;  // Placeholder - needs calibration
}

double RPM_RIGHT_calculator(double pwm_value)
{
    // TODO: Implement actual PWM to RPM conversion for right motor
    // This should be based on motor characterization data
    // Example: return (pwm_value * motor_constant + offset);
    return pwm_value * 1.0;  // Placeholder - needs calibration
}