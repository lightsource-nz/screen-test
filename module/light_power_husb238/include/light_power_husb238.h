#ifndef _LIGHT_POWER_HUSB238_H
#define _LIGHT_POWER_HUSB238_H

#include <light_power.h>

//   Hynetek HUSB238 USB Type-C Power Delivery SINK controller -- a "PD trigger": it
// negotiates a contract with a USB-C source and hands the selected voltage to whatever is
// downstream. It manages no rails of its own, which is why it is a light_power device rather
// than anything resembling a PMIC.
//
//   PROVENANCE, because it changes how much the numbers below should be trusted: this map
// comes from third-party libraries and a register-information sheet, NOT from a datasheet we
// hold. Everything structural in it was confirmed against real hardware on 2026-08-26 --
// the address, the eleven-register file, and the per-voltage detect flags, which appeared on
// exactly the five voltages a 45W charger offers and were clear on the 18V it does not.
// The current-code table below is corroborated arithmetically rather than directly: it
// yields 45W at both 15V/3.0A and 20V/2.25A independently, which a wrong table would not.
//
//   the part is marked HUSB238; a board carrying one may read HUSB328, which is the same
// part with the digits transposed.

#define HUSB238_I2C_ADDR                        0x08

//   the whole register file. Every one of these is read and written ONE BYTE AT A TIME: this
// part transfers a single byte per bus transaction, so the auto-incrementing multi-byte
// bursts that light_touch_cst816t and light_imu_qmi8658 use would be wrong here
#define HUSB238_REG_PD_STATUS0                  0x00
#define HUSB238_REG_PD_STATUS1                  0x01
#define HUSB238_REG_SRC_PDO_5V                  0x02
#define HUSB238_REG_SRC_PDO_9V                  0x03
#define HUSB238_REG_SRC_PDO_12V                 0x04
#define HUSB238_REG_SRC_PDO_15V                 0x05
#define HUSB238_REG_SRC_PDO_18V                 0x06
#define HUSB238_REG_SRC_PDO_20V                 0x07
#define HUSB238_REG_SRC_PDO_SEL                 0x08
#define HUSB238_REG_GO_COMMAND                  0x09

// the six SRC_PDO registers are contiguous from 5V upward, which is what lets the driver
// walk them by index rather than naming each one
#define HUSB238_PDO_FIRST_REG                   HUSB238_REG_SRC_PDO_5V
#define HUSB238_PDO_COUNT                       6

// SRC_PDO_nV layout: bit 7 says the source offers this voltage, bits 0-3 code the current
#define HUSB238_PDO_DETECTED                    0x80
#define HUSB238_PDO_CURRENT_MASK                0x0F

// PD_STATUS0: high nibble codes the negotiated voltage, low nibble the current
#define HUSB238_STATUS0_VOLTAGE_SHIFT           4
#define HUSB238_STATUS0_CURRENT_MASK            0x0F

// SRC_PDO_SEL: the PDO to request, in the high nibble, using the same voltage codes
#define HUSB238_PDO_SEL_SHIFT                   4

//   GO_COMMAND: the bottom five bits are a command. REQUEST_PDO acts on whatever SRC_PDO_SEL
// holds, so the two are always written as a pair and in that order
#define HUSB238_GO_REQUEST_PDO                  0x01

extern struct power_driver *light_power_driver_husb238();
//   io must already be set up for this device's address (HUSB238_I2C_ADDR). No reset pin
// argument: this part has no reset line, and none of the recovery machinery the touch
// drivers need applies -- an unpowered HUSB238 is silent because nothing is plugged into
// its USB-C port, which no amount of resetting will fix
extern struct power_device *light_power_husb238_create_device(uint8_t *name, struct io_context *io);

#endif
