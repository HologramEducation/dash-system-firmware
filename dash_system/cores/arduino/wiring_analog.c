/*
  wiring_analog.c - Wiring compatibility layer analog-mode
  functions with mods for Hologram Dash

  https://hologram.io

  Copyright (c) 2016 Konekt, Inc.  All rights reserved.


  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "Arduino.h"
#include "wiring_constants.h"
#include "hal/fsl_adc16_hal.h"
#include "hal/fsl_tpm_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

static int _readResolution = 10;
static adc16_resolution_t _ADCResolution = kAdc16ResolutionBitOfSingleEndAs10;
static int _writeResolution = 8;
static bool _calibrated = false;
static const uint8_t _resolution_map[] = {8,12,10,16};

int getAnalogReadResoultion(void)
{
    return _readResolution;
}

int getAnalogWriteResolution(void)
{
    return _writeResolution;
}

void analogReadResolution( int res )
{
    _readResolution = res ;
    if (res > 12)
    {
        _ADCResolution = kAdc16ResolutionBitOfSingleEndAs16;
    }
    else if (res > 10)
    {
        _ADCResolution = kAdc16ResolutionBitOfSingleEndAs12;
    }
    else if (res > 8)
    {
        _ADCResolution = kAdc16ResolutionBitOfSingleEndAs10;
    }
    else
    {
        _ADCResolution = kAdc16ResolutionBitOfSingleEndAs8;
    }
}

void analogWriteResolution( int res )
{
    _writeResolution = res ;
}

static inline uint32_t mapResolution( uint32_t value, uint32_t from, uint32_t to )
{
    if ( from == to )
    {
        return value ;
    }

    if ( from > to )
    {
        return value >> (from-to) ;
    }
    else
    {
        return value << (to-from) ;
    }
}

/*
 * Only VREF supported
 */
void analogReference( eAnalogReference ulMode )
{
}

static eAnalogSampleCycles adc_cycles = AS_CYCLES_1;

void analogReadSampleCycles(eAnalogSampleCycles cycles)
{
    switch(cycles)
    {
        case AS_CYCLES_1:
        case AS_CYCLES_4:
        case AS_CYCLES_10:
        case AS_CYCLES_16:
        case AS_CYCLES_24:
            if(cycles != adc_cycles)
            {
                _calibrated = false;
                adc_cycles = cycles;
            }
            break;
        default:
            break;
    }
}

static adc16_long_sample_cycle_t getCycles()
{
    switch(adc_cycles)
    {
        default:
        case AS_CYCLES_24: return kAdc16LongSampleCycleOf24;
        case AS_CYCLES_4:  return kAdc16LongSampleCycleOf4;
        case AS_CYCLES_10: return kAdc16LongSampleCycleOf10;
        case AS_CYCLES_16: return kAdc16LongSampleCycleOf16;
    }
}

static bool analogCalibrate(ADC_Type* adc)
{
    ADC16_HAL_SetAutoCalibrationCmd(adc, true);
    while(!ADC16_HAL_GetChnConvCompletedFlag(adc, 0U))
    {
        if(ADC16_HAL_GetAutoCalibrationFailedFlag(adc))
        {
            ADC16_HAL_SetAutoCalibrationCmd(adc, false);
            return false;
        }
    }
    ADC16_HAL_GetChnConvValue(adc, 0U);
    ADC16_HAL_SetAutoCalibrationCmd(adc, false);
    return true;
}

uint32_t analogRead( uint32_t ulPin )
{
    if(IO_NOT_ADC(ulPin)) return 0xFFFFFFFF;

    ADC_Type* adc = ADC0;
    sim_clock_gate_name_t gate_name = kSimClockGateAdc0;
    bool *calibrated = &_calibrated;
    SIM_HAL_EnableClock(SIM, gate_name);

    ADC16_HAL_Init(adc);

    adc16_converter_config_t config = {
        .lowPowerEnable = true,
        .clkDividerMode = kAdc16ClkDividerOf4,
        .longSampleTimeEnable = (adc_cycles != AS_CYCLES_1),
        .resolution = _ADCResolution,
        .clkSrc = kAdc16ClkSrcOfBusClk,
        .asyncClkEnable = false,
        .highSpeedEnable = false,
        .longSampleCycleMode = getCycles(),
        .hwTriggerEnable = false,
        .refVoltSrc = kAdc16RefVoltSrcOfVref,
        .continuousConvEnable = false,
        .dmaEnable = false
    };
    ADC16_HAL_ConfigConverter(adc, &config);
    if(!*calibrated)
        *calibrated = analogCalibrate(adc);

    if(PINS_PORT(ulPin) != NONE)
    {
        PORT_CLOCK_ENABLE(ulPin);
        PORT_SET_MUX_ANALOG(ulPin);
    }
    ADC16_HAL_SetChnMuxMode(adc, PINS_ADC_MUX(ulPin) == ADC_B ? kAdc16ChnMuxOfB : kAdc16ChnMuxOfA);

    adc16_chn_config_t chn_config = {
        .chnIdx = PINS_ADC_CHAN(ulPin),
        .convCompletedIntEnable = false,
        .diffConvEnable = false
    };
    ADC16_HAL_ConfigChn(adc, 0, &chn_config);
    while(!ADC16_HAL_GetChnConvCompletedFlag(adc, 0));
    uint16_t valueRead = ADC16_HAL_GetChnConvValue(adc, 0);

    return mapResolution(valueRead, _resolution_map[_ADCResolution], _readResolution);
}

#define PWM_TPM(inst)       ((inst) == PWM_2 ? TPM2 : ((inst) == PWM_1 ? TPM1 : TPM0))
#define PWM_GATE(inst)      ((inst) == PWM_2 ? kSimClockGateTpm2 : ((inst) == PWM_1 ? kSimClockGateTpm1 : kSimClockGateTpm0))
#define PWM_CHANNELS(inst)  (((inst) == PWM_0) ? 6 : 2)

void analogWrite( uint32_t ulPin, uint32_t ulValue )
{
    if(IO_NOT_VALID(ulPin)) return;

    if(IO_NOT_PWM(ulPin))
    {
        pinMode(ulPin, OUTPUT);
    	ulValue = mapResolution(ulValue, _writeResolution, 8);
    	if (ulValue < 128)
    		digitalWrite(ulPin, LOW);
    	else
    		digitalWrite(ulPin, HIGH);
    }
    else if(PINS_PWM_DAC(ulPin))
    {
        //treat as DAC
        DAC_Type *dacBase = DAC0;
        SIM_HAL_EnableClock(SIM, kSimClockGateDac0);
        uint8_t i;

        DAC_WR_SR(dacBase, 0U); /* Clear all flags. */
        DAC_WR_C0(dacBase, 0U);
        DAC_WR_C1(dacBase, 0U);
        DAC_WR_C2(dacBase, DAC_DATL_COUNT-1U);

        //Enable
        DAC_BWR_C0_DACEN(dacBase, 1U);

        uint8_t c0;

        c0 = DAC_RD_C0(dacBase);
        c0 &= ~(  DAC_C0_LPEN_MASK  );
        c0 |= DAC_C0_DACRFS_MASK;

        DAC_WR_C0(dacBase, c0);

        ulValue = mapResolution(ulValue, _writeResolution, 12);

        DAC_WR_DATL(DAC0, 0, (uint8_t)(0xFFU & ulValue) );
        DAC_BWR_DATH_DATA1(DAC0, 0, (uint8_t)((0xF00U & ulValue)>>8U) );
        DAC_BWR_C2_DACBFRP(DAC0, 0);
    }
    else
    {
        //treat as PWM
        sim_clock_gate_name_t gate = PWM_GATE(PINS_PWM_INST(ulPin));
        TPM_Type *tpmBase = PWM_TPM(PINS_PWM_INST(ulPin));
        uint32_t num_channels = PWM_CHANNELS(PINS_PWM_INST(ulPin));
        uint8_t channel = PINS_PWM_CHAN(ulPin);
        if(channel >= num_channels) return;

        if(!SIM_HAL_GetGateCmd(SIM, gate))
        {
            //Init
            SIM_HAL_EnableClock(SIM, gate);
            for(int i = 0; i < num_channels; i++)
            {
                TPM_WR_CnSC(tpmBase, i, 0);
                TPM_WR_CnV(tpmBase, i, 0);
            }
            TPM_WR_STATUS(tpmBase, TPM_STATUS_CH0F_MASK | TPM_STATUS_CH1F_MASK | TPM_STATUS_TOF_MASK);
            TPM_WR_CONF(tpmBase, 0);

            TPM_HAL_SetClockDiv(tpmBase, kTpmDividedBy1);
            CLOCK_HAL_SetTpmSrc(SIM, 0, kClockTpmSrcMcgIrClk);

            TPM_HAL_SetMod(tpmBase, SystemIRClock / (800u) - 1);
            TPM_HAL_SetCpwms(tpmBase, 0);


            // TPM_HAL_SetSyncMode(tpmBase, kFtmUseSoftwareTrig);
            //
            // TPM_HAL_SetTofFreq(tpmBase, 0);
            // TPM_HAL_SetWriteProtectionCmd(tpmBase, 0);
            // TPM_HAL_SetBdmMode(tpmBase, 0);
            //
            // //SetClock
            // TPM_HAL_SetClockPs(tpmBase, kFtmDividedBy1);
            //
            // TPM_HAL_ClearTimerOverflow(tpmBase);
            //
            // TPM_HAL_SetCounterInitVal(tpmBase, 0); //not per-channel
            //
            // TPM_HAL_SetMod(tpmBase, SystemCoreClock / (24000u) - 1);
            // TPM_HAL_SetCpwms(tpmBase, 0);
        }

        if(PINS_PORT(ulPin) != NONE)
        {
            PORT_CLOCK_ENABLE(ulPin);
            PORT_SET_MUX_PWM(ulPin);
        }

        // TPM_HAL_SetChnEdgeLevel(tpmBase, channel, 2);
        // TPM_HAL_SetChnMSnBAMode(tpmBase, channel, 2);


        TPM_HAL_DisableChn(tpmBase, channel);

        uint16_t uMod = TPM_HAL_GetMod(tpmBase);
        uint16_t uCnv = uMod * ulValue / (1 << _writeResolution);
        /* For 100% duty cycle */
        if(uCnv >= uMod)
        {
            uCnv = uMod + 1;
        }

        TPM_HAL_SetChnCountVal(tpmBase, channel, uCnv);


        /* Set the requested PWM mode */
        //TPM_HAL_EnablePwmMode(tpmBase, param, channel);


        /* Set the TPM clock */
        TPM_HAL_SetClockMode(tpmBase, kTpmClockSourceModuleClk);
        TPM_HAL_SetChnMsnbaElsnbaVal(tpmBase, channel, 0x28);


        // TPM_HAL_SetChnCountVal(tpmBase, channel, uCnv);
        // TPM_HAL_SetClockSource(tpmBase, kClock_source_TPM_SystemClk);
        // TPM_HAL_SetSoftwareTriggerCmd(tpmBase, true);
    }

}

#ifdef __cplusplus
}
#endif
