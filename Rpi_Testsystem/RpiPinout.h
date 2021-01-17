#ifndef RPIPINOUT_H
#define RPIPINOUT_H

// https ://de.pinout.xyz/pinout/wiringpi#
// WiringPi pins definition for test system

struct tPinHeaderWiringPi
{
	int Header_Pin;
	int WiringPi_Pin;
};

struct tSystemPins
{
	tPinHeaderWiringPi Master_Client_Pin;
	tPinHeaderWiringPi Connection_Pin;
	tPinHeaderWiringPi Send_Receive_Pin;
};

constexpr size_t MAX_NR_TEST_SYSTEMS = 9;

constexpr tSystemPins TEST_SYSTEM_PINS[MAX_NR_TEST_SYSTEMS] = {
	// Testsystem 1 pin definitions
	{{ 3, 8 }, { 5, 9 }, { 7, 7 }},
	// Testsystem 2 pin definitions
	{{ 8, 15 }, { 10, 16 }, { 12, 1 }},
	// Testsystem 3 pin definitions
	{{ 11, 0 }, { 13, 2 }, { 15, 3 }},
	// Testsystem 4 pin definitions
	{{ 16, 4 }, { 18, 5 }, { 22, 6 }},
	// Testsystem 5 pin definitions
	{{ 19, 12 }, { 21, 13 }, { 23, 14 }},
	// Testsystem 6 pin definitions
	{{ 24, 10 }, { 26, 11 }, { 28, 31 }},
	// Testsystem 7 pin definitions
	{{ 27, 30 }, { 29, 21 }, { 31, 22 }},
	// Testsystem 8 pin definitions
	{{ 33, 23 }, { 35, 24 }, { 37, 25 }},
	// Testsystem 9 pin definitions
	{{ 36, 27 }, { 38, 28 }, { 40, 29 }}
};

#endif