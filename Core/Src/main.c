/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2023 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "fatfs.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "stm32f1xx.h"
#include "BMP180.h"
#include "stdio.h"
#include "MPU6050.h"
#include "math.h"
#include "stdint.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define RAD_TO_DEG 57.29578
#define DEG_TO_RAD 0.017453
#define ALPHA 0.96                          // Complementary Filter alpha value
#define dt 0.00069                       	// check while loop time
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;
I2C_HandleTypeDef hi2c2;

SPI_HandleTypeDef hspi1;

TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;

UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
float temp, press, altitude;                 	// Define User Variables Here!
int second = 0;                               	// time out variance
int Parachute = 0;                       		// state of parachute 0 is not deployed
int start = 0;                               	// state of time out func

float Z_velocity = 0.0f, Z_velgap = 0.0f, Z_stack = 0.0f, Z_velmean = 0.0f;
float gyroAngleX = 0.0f, gyroAngleY = 0.0f; 	// pitch(X) and roll(Y) angle (Euler Angle) by gyroscope
float accelAngleX = 0.0f, accelAngleY = 0.0f; 	// pitch(X) and roll(Y) angle (Euler Angle) by AccelerateScope
float compAngleX = 0.0f, compAngleY = 0.0f; 	// Complementary Filter result angle        URL https://blog.naver.com/intheglass14/222777512235 https://yjhtpi.tistory.com/352
float Rocket_vector[3] = { 0.0f };              // rocket vector(attitude) init

float vectorG[3] = {0.0f, 0.0f, -9.81f};
float Z_unitvector[3] = { 0.0f, 0.0f, 1.0f };   // Z-axis vector
float magnitudeRocket = 0.0f;
float magnitudeG = 0.0f;
float cosTheta = 0.0f;
float dotProduct = 0.0f;
float Rocket_Angle_Z = 0.0f;    					// result of dot product Rocket Vector with Z-axis
float a, b, c = 0.0f;                         	// just acos variables
typedef struct
{
	float x;
	float y;
	float z;
}vector;

uint32_t startTick, endTick, elapsedTicks, costTime_us; //just check while loop time
//-------------------------------- quaternion
float q0 = 1.0f;
float q1 = 0.0f;
float q2 = 0.0f;
float q3 = 0.0f;
float norm; 									// Acceleration
float dq0, dq1, dq2, dq3; 						// variance quater
float qDot0, qDot1, qDot2, qDot3;				// by acc
float pDot0, pDot1, pDot2, pDot3;				// by gyro
float lambda;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_I2C1_Init(void);
static void MX_I2C2_Init(void);
static void MX_SPI1_Init(void);
static void MX_TIM2_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_TIM3_Init(void);
/* USER CODE BEGIN PFP */
void SysTick_Init(void);               // systick  = like tic toc func in matlab
void SysTick_Delay_us(uint32_t us);
void setInitialQuaternion(float yaw, float pitch, float roll); //yaw_z, pitch_y, roll_x
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

// to use printf
#ifdef __GNUC__
/* With GCC/RAISONANCE, small printf (option LD Linker->Libraries->Small printf
 set to 'Yes') calls __io_putchar() */
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif /* __GNUC__ */

PUTCHAR_PROTOTYPE {
	/* Place your implementation of fputc here */
	/* e.g. write a character to the EVAL_COM1 and Loop until the end of transmission */
	HAL_UART_Transmit(&huart2, (uint8_t*) &ch, 1, 0xFFFF);
	return ch;
}
// use printf

// sdcard part
FATFS fs; // file system
FIL fil;  // File
FILINFO fno;
FRESULT fresult; // result
UINT br, bw;     // File read/write count

/**** capacity related *****/
FATFS *pfs;
DWORD fre_clust;
uint32_t total, free_space;
char str[16]; // Enough to hold all numbers up to 32-bit int
// sdcard part

// mpu6050
RawData_Def myAccelRaw, myGyroRaw;
ScaledData_Def myAccelScaled, myGyroScaled, accAverage;
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
	MPU_ConfigTypeDef myMpuConfig;
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */
  SysTick_Init();
  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART2_UART_Init();
  MX_I2C1_Init();
  MX_I2C2_Init();
  MX_SPI1_Init();
  MX_TIM2_Init();
  MX_FATFS_Init();
  MX_USART1_UART_Init();
  MX_TIM3_Init();
  /* USER CODE BEGIN 2 */
	initialBMP180();
	MPU6050_Init(&hi2c1);
	myMpuConfig.Accel_Full_Scale = AFS_SEL_16g; 		// full scale setting
	myMpuConfig.ClockSource = Internal_8MHz;
	myMpuConfig.CONFIG_DLPF = DLPF_184A_188G_Hz;
	myMpuConfig.Gyro_Full_Scale = FS_SEL_500;
	myMpuConfig.Sleep_Mode_Bit = 0;
	MPU6050_Config(&myMpuConfig);
	HAL_TIM_Base_Start(&htim2);
	HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
	HAL_TIM_Base_Start_IT(&htim3);

	fresult = f_mount(&fs, "/", 1);
	/* Check free space */
	f_getfree("", &fre_clust, &pfs);
	total = (uint32_t) ((pfs->n_fatent - 2) * pfs->csize * 0.5);
	free_space = (uint32_t) (fre_clust * pfs->csize * 0.5);
	/* Open file to write/ create a file if it doesn't exist */
	fresult = f_open(&fil, "file1.txt", FA_OPEN_ALWAYS | FA_READ | FA_WRITE);
	/* Writing text */

	float realAcc_x,realAcc_y,realAcc_z;
	float desiredAngle = 30.0;
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
	while (1) {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
		startTick = SysTick->VAL;             	// tic (check loop time)
		// bmp180 part
		temp = readTrueTemp();
		press = readTruePress(0);
		altitude = readTrueAltitude(0); 		// altitude = initAltitude - readTrueAltitude(0); will come here after confirm.
		// bmp180 part

		// mpu6050 part
		MPU6050_Get_Accel_Scale(&myAccelScaled);
		MPU6050_Get_Gyro_Scale(&myGyroScaled);

		// mpu6050 part
		if(start==0&&myAccelScaled.z<=1.5f){
			for(int count=0;count<3;count++){
				accAverage.x=0,accAverage.y=0,accAverage.z=0;
				accAverage.x=+myAccelScaled.x;
				accAverage.y=+myAccelScaled.y;
				accAverage.z=+myAccelScaled.z;
			}
			accAverage.x=accAverage.x/3.0f;
			accAverage.y=accAverage.y/3.0f;
			accAverage.z=accAverage.z/3.0f;		//moving mean filter
			gyroAngleX = atan2f(accAverage.y,		// RAD pitch and roll by acc scope
					sqrtf(accAverage.x * accAverage.x
							+ accAverage.z * accAverage.z));
			gyroAngleY = atan2f(-accAverage.x, 	//-myacc.x to myacc.x
					sqrtf(accAverage.y * accAverage.y
							+ accAverage.z * accAverage.z));

			setInitialQuaternion(0.0f, gyroAngleY,gyroAngleX);

			continue;		//start trigger
		}
		else
			start=1;
		/*                        this code will be run after remove gravity element from accelerate scope and add time out start point here
		 if (start == 0 || myAccelScaled.z >= 15.0)
		 {
		 Parachute = 1;
		 start = 1;
		 printf("Start deploying parachute system\r\n");
		 } */

		norm = sqrt(myAccelScaled.x * myAccelScaled.x + myAccelScaled.y * myAccelScaled.y + myAccelScaled.z * myAccelScaled.z);

		// accerlation variance minimum setting	가속도가 충분히 큰 경우에만 사용한
		if (norm > 0.1f) {
		    // 가속도를 단위 벡터로 정규화한다
			norm = 1.0f / norm;
			accAverage.x = myAccelScaled.x;
			accAverage.y = myAccelScaled.y;
			accAverage.z = myAccelScaled.z;
		    accAverage.x *= norm;
		    accAverage.y *= norm;
		    accAverage.z *= norm;

		    // calculate rotation matrix from present quaternion 현재의 쿼터니온으로부터 회전행렬의 일부를 계산한다
		    float _2q0 = 2.0f * q0;
		    float _2q1 = 2.0f * q1;
		    float _2q2 = 2.0f * q2;
		    float _2q3 = 2.0f * q3;
		    float _4q0 = 4.0f * q0;
		    float _4q1 = 4.0f * q1;
		    float _4q2 = 4.0f * q2;
		    float _8q1 = 8.0f * q1;
		    float _8q2 = 8.0f * q2;
		    float q0q0 = q0 * q0;
		    float q1q1 = q1 * q1;
		    float q2q2 = q2 * q2;
		    float q3q3 = q3 * q3;

		    // calculate variance of quaternion by accerlate scope (gradiant descent algorithm) 가속도에 의한 쿼터니온의 변화량을 계산한다 (그라디언트 디센트 알고리즘)
		    qDot0 = _4q0 * q2q2 + _2q2 * accAverage.x + _4q0 * q1q1 - _2q1 * accAverage.y;
		    qDot1 = _4q1 * q3q3 - _2q3 * accAverage.x + 4.0f * q0q0 * q1 - _2q0 * accAverage.y - _4q1 + _8q1 * q1q1 + _8q1 * q2q2 + _4q1 * accAverage.z;
		    qDot2 = 4.0f * q0q0 * q2 + _2q0 * accAverage.x + _4q2 * q3q3 - _2q3 * accAverage.y - _4q2 + _8q2 * q1q1 + _8q2 * q2q2 + _4q2 * accAverage.z;
		    qDot3 = 4.0f * q1q1 * q3 - _2q1 * accAverage.x + 4.0f * q2q2 * q3 - _2q2 * accAverage.y;

		    // calc size of gradiant 그라디언트의 크기를 계산한다
		    norm = sqrt(qDot0 * qDot0 + qDot1 * qDot1 + qDot2 * qDot2 + qDot3 * qDot3);

		    // normalize gradiant to unit vector 그라디언트를 단위 벡터로 정규화한다
		    norm = 1.0f / norm;
		    qDot0 *= norm;
		    qDot1 *= norm;
		    qDot2 *= norm;
		    qDot3 *= norm;
		}

		// calculate variance of quaternion by gyroscope 각속도에 의한 쿼터니온의 변화량을 계산한다
		pDot0 = -0.5f * (q1 * myGyroScaled.x + q2 * myGyroScaled.y + q3 * myGyroScaled.z);
		pDot1 =  0.5f * (q0 * myGyroScaled.x + q2 * myGyroScaled.z - q3 * myGyroScaled.y);
		pDot2 =  0.5f * (q0 * myGyroScaled.y - q1 * myGyroScaled.z + q3 * myGyroScaled.x);
		pDot3 =  0.5f * (q0 * myGyroScaled.z + q1 * myGyroScaled.y - q2 * myGyroScaled.x);

		// weight (complementary filter) 가속도와 각속도에 의한 쿼터니온의 변화량을 가중평균한다
		dq0 = pDot0 - (1-ALPHA) * qDot0;
		dq1 = pDot1 - (1-ALPHA) * qDot1;
		dq2 = pDot2 - (1-ALPHA) * qDot2;
		dq3 = pDot3 - (1-ALPHA) * qDot3;

		// update quaternion 쿼터니온을 업데이트한다
		q0 += dq0 * dt;
		q1 += dq1 * dt;
		q2 += dq2 * dt;
		q3 += dq3 * dt;

		// normalize quarternion 쿼터니온을 정규화한다
		norm = sqrt(q0*q0 + q1*q1 + q2*q2 + q3*q3);
		norm = 1.0f / norm;
		q0 *= norm;
		q1 *= norm;
		q2 *= norm;
		q3 *= norm;

		// quaternion to rocket vector 쿼터니온을 방향벡터로 변환한다
		Rocket_vector[0] = 2.0f*(q1*q3 - q0*q2); // x축
		Rocket_vector[1] = 2.0f*(q0*q1 + q2*q3); // y축
		Rocket_vector[2] = 1.0 - 2.0 * (q1 * q1 + q2 * q2); // z component
//		printf("%f, %f, %f\r\n",Rocket_vector[0],Rocket_vector[1],Rocket_vector[2]);
		// elimination gravity by minJun (Fail)
		dotProduct = Rocket_vector[0]*vectorG[0]+Rocket_vector[1]*vectorG[1]+Rocket_vector[2]*vectorG[2];
		magnitudeRocket = sqrt(Rocket_vector[0]*Rocket_vector[0] + Rocket_vector[1]*Rocket_vector[1] + Rocket_vector[2]*Rocket_vector[2]);
		magnitudeG = sqrt(vectorG[0]*vectorG[0] + vectorG[1]*vectorG[1] + vectorG[2]*vectorG[2]);
		cosTheta = dotProduct / (magnitudeRocket * magnitudeG);
		realAcc_x = myAccelScaled.x - 9.81*Rocket_vector[0]*cosTheta;
		realAcc_y = myAccelScaled.y - 9.81*Rocket_vector[1]*cosTheta;
		realAcc_z = myAccelScaled.z - 9.81*Rocket_vector[2]*cosTheta;

		a = Rocket_vector[0] * Z_unitvector[0]
						+ Rocket_vector[1] * Z_unitvector[1]
						+ Rocket_vector[2] * Z_unitvector[2];
		b = sqrt(Rocket_vector[0] * Rocket_vector[0]
								+ Rocket_vector[1] * Rocket_vector[1]
								+ Rocket_vector[2] * Rocket_vector[2]);
		c = sqrt(Z_unitvector[0] * Z_unitvector[0]
								+ Z_unitvector[1] * Z_unitvector[1]
								+ Z_unitvector[2] * Z_unitvector[2]);
		// calc Rocket attitude (RAD) 로켓의 기울기 정도를 계산한다 (라디안)
		Rocket_Angle_Z = (float)acos(a / (b * c));

//		printf("RocketAngle : %.2f, Acc: %.2f %.2f %.2f \r\n",Rocket_Angle_Z*RAD_TO_DEG,realAcc_x,realAcc_y,realAcc_z);					// test code not pass

		//calculate acc comp by gravity
		if (myGyroScaled.z*dt > (M_PI / 4)) {
			//calc with sine    /use rho and thea
			lambda = (float)atan(tan((double)myGyroScaled.x*dt) / sin((double)myGyroScaled.z*dt));
		}
		else {
			//calc with cosine  /use pi and thea
			lambda = (float)atan(tan((double)myGyroScaled.y*dt) / cos((double)myGyroScaled.z*dt));
		}

		//with lambda, we should calculate gravity comp of each axis.
		//gravity = 9.81m/s^2
		float gravity_x = 9.81 * sin((double)lambda) * cos((double)myGyroScaled.z*dt);
		float gravity_y = 9.81* sin((double)lambda) * sin((double)myGyroScaled.z*dt);
		float gravity_z = 9.81* cos((double)lambda);


		//store acc to matrix
		float accx = myAccelScaled.x - gravity_x;
		float accy = myAccelScaled.y - gravity_y;
		float accz = myAccelScaled.z - gravity_z;
		printf("RocketAngle : %.2f, Acc: %.2f %.2f %.2f \r\n",Rocket_Angle_Z*RAD_TO_DEG,accx,accy,accz);					// test code not pass
		HAL_Delay(50);
//		float accZ_rot = -accZ_raw * sin(compAngleY) + accY_raw * cos(compAngleY);
//		printf("pure Z acc : %.2f\r\n", accZ_rot); 						// test code not pass

//		Z_stack -= Z_velocity;             								// delete pre-prev vel
//		Z_velocity += accZ_rot * dt;       								// calc Zvelocity
//		Z_stack += Z_velocity;             								// add updated vel
//		Z_velmean = Z_stack / 2;           								// mean of prev and
//		Z_velgap = Z_velocity - Z_velmean;

		// deploy parachute part
		if (Parachute == 0)                           					// == 1 ( 0 is just test )
		{
			if (altitude >= 380) {
				Parachute = 0;
				printf("deploy parachute: altitude\r\n");
			}
			else if (Rocket_Angle_Z*RAD_TO_DEG > desiredAngle ) {
				Parachute = 0;
				printf("deploy parachute: Desired Angle\r\n");
			}
			// if (Z_velgap <= 0.1f)
			// {
			//   Parachute = 0;
			//   printf("deploy parachute : Z velocity");
			// }
			else if (second >= 4) // 10 s
			{
				Parachute = 0;
				printf("deploy parachute : time out\r\n");
			}
		}

		// sdcard part
		char buffer[100]; // Buffer to hold string

		sprintf(buffer,
				"altitude: %.2f Gyro: X=%.2f Y=%.2f Z=%.2f Accel: X=%.2f Y=%.2f Z=%.2f\r\n",
				altitude, myGyroScaled.x, myGyroScaled.y, myGyroScaled.z,
				myAccelScaled.x, myAccelScaled.y, myAccelScaled.z);
		if (fresult == FR_OK) {
			fresult = f_write(&fil, buffer, strlen(buffer), &bw); // Write the string to file
			f_sync(&fil);                    // Ensure data is written and saved
		}
		endTick = SysTick->VAL;             	// tic (check loop time)
		// SysTick은 다운 카운터이므로 startTick > endTick
		elapsedTicks = startTick > endTick ? startTick - endTick : startTick + (0xFFFFFF - endTick);
		costTime_us = (elapsedTicks * 8) / (64000000 / 1000000);

//		printf("%lu\r\n", costTime_us);
		HAL_Delay(1);
		// sdcard part
	}
	// sdcard part
	if (fresult == FR_OK) {
		fresult = f_close(&fil);
	}
	// sdcard part

  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI_DIV2;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL16;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief I2C2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C2_Init(void)
{

  /* USER CODE BEGIN I2C2_Init 0 */

  /* USER CODE END I2C2_Init 0 */

  /* USER CODE BEGIN I2C2_Init 1 */

  /* USER CODE END I2C2_Init 1 */
  hi2c2.Instance = I2C2;
  hi2c2.Init.ClockSpeed = 100000;
  hi2c2.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c2.Init.OwnAddress1 = 0;
  hi2c2.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c2.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c2.Init.OwnAddress2 = 0;
  hi2c2.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c2.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C2_Init 2 */

  /* USER CODE END I2C2_Init 2 */

}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 1280 - 1;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 1000 - 1;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */
  HAL_TIM_MspPostInit(&htim2);

}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 6400 - 1;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 10000 - 1;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_OC_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_TIMING;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_OC_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */
  HAL_TIM_MspPostInit(&htim3);

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : PB0 */
  GPIO_InitStruct.Pin = GPIO_PIN_0;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) // time out private function
{
	if (htim->Instance == TIM3) {
		second++;
		if (second == 5) {
			//      printf("timeout ok\r\n");
			second = 0;
		}
	}
}

void SysTick_Init(void)                      // tic toc function - once of while
{
	SysTick->LOAD = 0xFFFFFF;                   // set max value
	SysTick->VAL = 0;                           // init present count to 0
	SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk |
	SysTick_CTRL_ENABLE_Msk;    // timer start using CPU Clock
}

void SysTick_Delay_us(uint32_t us)            // tic toc function
{
	SysTick->LOAD = us * (64000000 / 8 / 1000000) - 1;
	SysTick->VAL = 0;
	while (!(SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk))
		;
}
void setInitialQuaternion(float yaw, float pitch, float roll) { //yaw_z, pitch_y, roll_x
    float cy = cos(yaw * 0.5);
    float sy = sin(yaw * 0.5);
    float cp = cos(pitch * 0.5);
    float sp = sin(pitch * 0.5);
    float cr = cos(roll * 0.5);
    float sr = sin(roll * 0.5);

    q0 = cy * cp * cr + sy * sp * sr; // q0
    q1 = cy * cp * sr - sy * sp * cr; // q1
    q2 = sy * cp * sr + cy * sp * cr; // q2
    q3 = sy * cp * cr - cy * sp * sr; // q3
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
	/* User can add his own implementation to report the HAL error return state */
	__disable_irq();
	while (1) {
	}
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
