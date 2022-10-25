/*! ------------------------------------------------------------------------------------------------------------------
 * @file	instance.c
 * @brief	application level message exchange for ranging demo
 *
 * @attention
 * 
 * Copyright 2008, 2013 (c) DecaWave Ltd, Dublin, Ireland.
 *
 * All rights reserved.
 *
 */

#include "compiler.h"
#include "deca_spinit.h"
#include "instance.h"
#include "deca_util.h"
#include "deca_device_api.h"



// -------------------------------------------------------------------------------------------------------------------

extern instance_data_t instance_data[NUM_INST] ;

// -------------------------------------------------------------------------------------------------------------------
#define LONG_LINE_LEN   250
char longline[LONG_LINE_LEN+6] ;

uint8 msgbufferidx = 0;
uint8 msgbuffer[NUM_MSGBUFF_LINES][STAT_LINE_MSGBUF_LENGTH]; 
int msgbufferlen[NUM_MSGBUFF_LINES];


double arrRSL[10];
double arrSNR[10];
int index = 0;
int new_range = 0;
// -------------------------------------------------------------------------------------------------------------------
// Functions
// -------------------------------------------------------------------------------------------------------------------


void clearlogmsgbuffer(void)
{
    int i; 
    msgbufferidx = 0;
    for (i = 0 ; i < NUM_MSGBUFF_LINES ; i++) msgbufferlen[i] = 0 ;
}


void copybufftoscrline(char *scrline, char *buff, int length)
{
    int i ;
    //char c ;
	int j = 0;

	j += sprintf_s(&scrline[j], 10, "(%d) ", length);

    for (i = 0 ; i < length ; i++)                  // copy up to STAT_LINE_BUF_LENGTH chars
    {
		j += sprintf_s(&scrline[j],4,"%02X", (uint8) buff[i]);
    }

    for (; (i < STAT_LINE_MSGBUF_LENGTH-1) && (j < STAT_LINE_MSGBUF_LENGTH-1) ; i++)                        // for remainder of line fill with spaces.
    {
		scrline[j++] = ' ' ;
    }

	scrline[STAT_LINE_MSGBUF_LENGTH-1] = 0 ;                           // ensure it is nul terminated
}

void copylonglinetoscrline(char *scrline)
{
    int i ;
    char c ;

    for (i = 0 ; i < STAT_LINE_BUF_LENGTH-1 ; i++)                  // copy up to STAT_LINE_BUF_LENGTH chars
    {
        c = longline[i] ;
        if (c == '\0') break ;                                      //  stop if string termination is reached
        scrline[i] = c ;
    }

    for (; i < STAT_LINE_BUF_LENGTH-1 ; i++)                        // for remainder of line fill with spaces.
    {
        scrline[i] = ' ' ;
    }
    scrline[STAT_LINE_BUF_LENGTH-1] = 0 ;                           // ensure it is nul terminated
}

void copylongline2scrline(char *scrla,char *scrlb)                  // allow copy to wrap into a 2nd status line
{
    int i, j ;
    char c ;

    for (i = 0 ; i < STAT_LINE_BUF_LENGTH-1 ; i++)                  // copy up to STAT_LINE_BUF_LENGTH chars
    {
        c = longline[i] ;
        if (c == '\0') break ;                                      //  stop if string termination is reached
        scrla[i] = c ;
    }

    for (j = i; j < STAT_LINE_BUF_LENGTH-1 ; j++)                   // for remainder of line fill with spaces.
    {
        scrla[j] = ' ' ;
    }
    scrla[STAT_LINE_BUF_LENGTH-1] = 0 ;                             // ensure it is nul terminated

    // if more text available put into second line

    for (j = 0 ; j < STAT_LINE_BUF_LENGTH-1 ; j++)                  // copy up to STAT_LINE_BUF_LENGTH chars
    {
        c = longline[i++] ;
        if (c == '\0') break ;                                      //  stop if string termination is reached
        scrlb[j] = c ;
    }

    for (; j < STAT_LINE_BUF_LENGTH-1 ; j++)                        // for remainder of line fill with spaces.
    {
        scrlb[j] = ' ' ;
    }
    scrlb[STAT_LINE_BUF_LENGTH-1] = 0 ;                             // ensure it is nul terminated

}


void reportTOF(instance_data_t *inst)
{
        double distance = 0;
		double distance_to_correct = 0;
		double raw_distance;
		double bias_correction;
        double tof = 0;
        double stdev = 0;
        double ltave;
        int64 tofi ;

        // check for negative results and accept them making them proper negative integers

        tofi = inst->tof ;                          // make it signed
        if (tofi > 0x007FFFFFFFFF)                          // MP counter is 40 bits,  close up TOF may be negative
        {
            tofi -= 0x010000000000 ;                       // subtract fill 40 bit range to make it negative
        }

        // convert into seconds (as a signed floating point number) and then into a distance

        tof = convertdevicetimetosec(tofi);
        distance = tof * SPEED_OF_LIGHT;

        //printf("\ntof: %016I64X  tofi: %I64i  tofsec: %e  unfiltered dist: %f\n",inst->tof, tofi, tof, distance) ;

#if (CORRECT_RANGE_BIAS == 1)
        //for the 6.81Mb data rate we assume gating gain of 6dB is used,
        //thus a different range bias needs to be applied
        //if(inst->configData.dataRate == DWT_BR_6M8)
        if(inst->configData.smartPowerEn)
        {
        	//1.31 for channel 2 and 1.51 for channel 5
        	if(inst->configData.chan == 5)
        	{
        		distance_to_correct = distance/1.51;
        	}
        	else //channel 2
        	{
        		distance_to_correct = distance/1.31;
			}
        }
        else
        {
        	distance_to_correct = distance;
        }

		bias_correction = dwt_getrangebias(inst->configData.chan, (float)distance_to_correct, inst->configData.prf);
		raw_distance = distance;
		distance        = distance-bias_correction;
		tof = distance / SPEED_OF_LIGHT;                           // and figure out new corrected time of flight
#else
		bias_correction = 0.0;
#endif


#if ((DECA_ACCUM_LOG_SUPPORT==1) || (DECA_LOG_ENABLE==1))
		if ((inst->dwlogdata.accumLogging == LOG_ALL_ACCUM) || (inst->dwlogdata.accumLogging == LOG_ALL_NOACCUM))
		{
			fprintf(inst->dwlogdata.accumLogFile, "\n%s ToF: %-3.3f ns Dist: %-3.6f m DistRaw: %-3.6f m DistScal: %-3.6f m Bias: %0.3f m ClockOffset: %-2.3f ppm\n", ((inst->mode == TAG) ? "Tag" : "Anchor"), 
				tof*1e9, distance, raw_distance, distance_to_correct, bias_correction, inst->clockOffset);
		}
#else
		printf("%s ToF: %-3.3f ns Dist: %-3.6f m DistRaw: %-3.6f m DistScal: %-3.6f m Bias: %0.3f m ClockOffset: %-2.3f ppm\n", ((inst->mode == TAG) ? "Tag" : "Anchor"), 
			tof*1e9, distance, raw_distance, distance_to_correct, bias_correction, inst->clockOffset);
#endif
        if ((distance < 0) || (distance > 20000.000))    // discount any items with error
		{
            return;
		}

        inst->longTermRangeSum+= distance ;
        inst->longTermRangeCount++ ;                                // for computing a long term average
        ltave = inst->longTermRangeSum / inst->longTermRangeCount ;

        inst->adist[inst->tofindex++] = distance ;
        inst->adist4[(inst->tofindex)&0x03] = distance ;            // for average of last four 
		inst->astdevdist[inst->tofstdevindex++] = distance ;

        inst->idistance = distance ;
        inst->shortTermMean = 0.0 ;
        inst->shortTermMeanNumber = 0 ;

		new_range = 1;

        if(distance < inst->idistmin)
            inst->idistmin = distance;

        if(distance > inst->idistmax)
            inst->idistmax = distance;

        if(inst->tofindex == RTD_MED_SZ) inst->tofindex = 0;
		if(inst->tofstdevindex == RTD_STDEV_SZ) inst->tofstdevindex = 0;

        if(inst->tofcount == RTD_MED_SZ)
        {
            int i;
            double avg;
            double avg4;

            avg = 0;
            for(i = 0; i < inst->tofcount; i++)
            {
                avg += inst->adist[i];
            }
            avg /= inst->tofcount;
            inst->shortTermMean = avg  ;

            avg4 = 0;
            for(i = 0; i < 4; i++)
            {
                avg4 += inst->adist4[i];
            }
            avg4 /= 4.0 ;
            inst->shortTermMean4 = avg4  ;

            inst->shortTermMeanNumber = inst->tofcount ;
           
			if(inst->longTermRangeCount >= RTD_STDEV_SZ)
			{ // calculate STDEV
                int i ; double sum = 0 ; double diff ;
                for (i = 0 ; i < RTD_STDEV_SZ ; i++)
                {
                    diff = inst->astdevdist[i] - avg ;
                    sum += diff * diff ;
                }
                stdev = sqrt(sum/RTD_STDEV_SZ) ;
            } 

            if (inst->dispFeetInchesEnable)                         // display with feet and inches
            {
                double idistin = avg / 0.0254 ;
                double idistft = floor(idistin / 12 ) ;
                int idistini = (int)(idistin - idistft * 12) ;
                int idistfti = (int)(idistft) ;
                sprintf_s(longline,LONG_LINE_LEN,
                "Mean(%i): Dist: %-3.2f m (%i' %i\" ) STDEV(%i): %-3.2f cm                                                                                 ",
                RTD_MED_SZ, avg, idistfti, idistini, RTD_STDEV_SZ, stdev*100
                ) ;
                }
                else
                {
                sprintf_s(longline,LONG_LINE_LEN,
                "Mean(%i): Dist: %-3.2f m  STDEV(%i): %-3.2f cm                                                                                               ",
                RTD_MED_SZ, avg, RTD_STDEV_SZ, stdev*100
                ) ;
            }
                copylonglinetoscrline(inst->statusrep.scr[STAT_LINE_MEAN]) ;
        }
        else
            inst->tofcount++;

        if (inst->dispFeetInchesEnable)                         // display with feet and inches
        {
            double idistin = distance / 0.0254 ;
            double idistft = floor(idistin / 12 ) ;
            int idistini = (int)(idistin - idistft * 12) ;
            int idistfti = (int)(idistft) ;
            sprintf_s(longline,LONG_LINE_LEN,
            "Instant: ToF: %-3.3f ns Dist: %-3.3f m (%i' %i\" )                                                                                           ",
            tof*1e9, distance, idistfti, idistini
            ) ;
        }
        else
        {
            sprintf_s(longline,LONG_LINE_LEN,
            "Instant: ToF: %-3.3f ns Dist: %-3.3f m                                                                                      ",
            tof*1e9, distance 
            ) ;
        }
        copylonglinetoscrline(inst->statusrep.scr[STAT_LINE_INST]) ;

		if (inst->dispClkOffset)                         // display clock offset
		{
            if (inst->clockOffset < 0)
			    sprintf_s(longline, LONG_LINE_LEN, "Long Term Average = %-3.6f m  (over %i)    Clock Offset: anc faster by %-2.3f ppm                               ",
			              ltave, inst->longTermRangeCount, -inst->clockOffset);
            else
                sprintf_s(longline, LONG_LINE_LEN, "Long Term Average = %-3.6f m  (over %i)    Clock Offset: anc slower by %-2.3f ppm                               ",
                          ltave, inst->longTermRangeCount, inst->clockOffset);
		}
		else
		{
			sprintf_s(longline,LONG_LINE_LEN,"Long Term Average = %-3.6f m  (over %i)                                                                         ",
			ltave,inst->longTermRangeCount) ;
		}
        copylonglinetoscrline(inst->statusrep.scr[STAT_LINE_LTAVE]) ;

        inst->statusrep.changed = 1 ;
    return ;
}// end of reportTOF

void clearreportTOF(statusReport_t *st)
{
        sprintf_s(longline,LONG_LINE_LEN,
                "Mean(%i): Dist: %-3.2f m STDEV: %-3.2f cm                                                                                                ",
                0, 0.0f
                ) ;

        copylonglinetoscrline(st->scr[STAT_LINE_MEAN]) ;
 

        sprintf_s(longline,LONG_LINE_LEN,
            "Instant: ToF: %-3.3f ns Dist: %-3.3f m                                                                                                       ",
            0.0f, 0.0f
            ) ;

        copylonglinetoscrline(st->scr[STAT_LINE_INST]) ;

        sprintf_s(longline,LONG_LINE_LEN,"Long Term Average = %-3.6f m  (over %i)                                                                         ",
            0.0f,0) ;
        copylonglinetoscrline(st->scr[STAT_LINE_LTAVE]) ;

        st->changed = 1 ;
    return ;
}// end of clearreportTOF

// -------------------------------------------------------------------------------------------------------------------
//
void instancegetcurrentrangeinginfo(currentRangeingInfo_t *info) 
{
    int instance = 0 ;

    info->idist = instance_data[instance].idistance ;
    info->mean4= instance_data[instance].shortTermMean4 ;
    info->mean = instance_data[instance].shortTermMean ;
    info->numAveraged = instance_data[instance].shortTermMeanNumber ;
	info->lastRangeReportTime = instance_data[instance].lastReportTime ;
}


// -------------------------------------------------------------------------------------------------------------------
// function to read event counters (good, bad frames; timeouts; txframes; errors)
//
int instancereadevents(dwt_deviceentcnts_t *itemp)
{
#if DECA_ERROR_LOGGING ==1
	dwt_readeventcounters(itemp);
#endif
    return 1;

}

// Read low level 32-bit register

int instancelowlevelreadreg(int regID,int regOffset,unsigned int *retRegValue)
{
	*retRegValue = dwt_read32bitoffsetreg(regID,regOffset) ;
    return DWT_SUCCESS ;

} // end instancelowlevelreadreg()



// Write low level 32-bit register value

int instancelowlevelwritereg(int regID,int regOffset,unsigned int regValue)
{
    return dwt_write32bitoffsetreg(regID,regOffset,regValue) ;   // Acces Register via PHY

} // end instancelowlevelwritereg()

// -------------------------------------------------------------------------------------------------------------------
// function to enable status report of distance in feet and inches
//
void instancesetdisplayfeetinches(int feetInchesEnable)
{
    int instance = 0 ;
    instance_data[instance].dispFeetInchesEnable = feetInchesEnable ;
}

// -------------------------------------------------------------------------------------------------------------------
// function to enable clock offset display
//
void instancesetdisplayclockoffset(int clockOff)
{
    int instance = 0 ;
    instance_data[instance].dispClkOffset = clockOff ;
}

void instancedisplaynewstatusdata(instance_data_t *inst, dwt_deviceentcnts_t *itemp)
{
	int cnt ;

#if DECA_ERROR_LOGGING==1
    cnt = sprintf_s(longline,LONG_LINE_LEN,
        "TX=%-8i RX=%-8i CE=%-7i RSE=%-7i HE=%-7i STO=%-7i FFE=%-7i TO=%-7i L=%-7i",
		itemp->TXF,
        itemp->CRCG,
        itemp->CRCB,
        itemp->RSL,
        itemp->PHE,
        itemp->SFDTO,
		itemp->ARFE,
        inst->rxTimeouts, (inst->lateTX + inst->lateRX)
        );
#else
    cnt = sprintf_s(longline,LONG_LINE_LEN,
        "TX=%-9i RX=%-9i TO=%-7i L=%-7i",
        itemp->TXF,
        itemp->CRCG,
        inst->rxTimeouts, (inst->lateTX + inst->lateRX)
        );
#endif
    copylonglinetoscrline(inst->statusrep.scr[STAT_LINE_MSGCNT]) ;
}

void instancelogrxblinkdata(instance_data_t *inst, event_data_t *dw_event)
{
	uint32 taglo = 0;
	uint32 taghi = 0;
	double voltage = 0;
	double temperature = 0;
	
	memcpy(&taglo, &(dw_event->msgu.rxblinkmsgdw.tagID[0]), 4);
	memcpy(&taghi, &(dw_event->msgu.rxblinkmsgdw.tagID[4]), 4);

	//(Vbat + 384)/170 to get Volts
	voltage = dw_event->msgu.rxblinkmsgdw.vbat ;
	voltage = (voltage + 384) / 170 ;

	temperature = dw_event->msgu.rxblinkmsgdw.temp ;

	sprintf_s(longline,LONG_LINE_LEN,"Blink Data: 0x%08X%08X %3.2f V %3.1f C %02X", taghi, taglo, voltage, temperature, dw_event->msgu.rxblinkmsgdw.gpio);
	copylongline2scrline(inst->statusrep.scr[STAT_LINE_LTAVE],inst->statusrep.scr[STAT_LINE_USER_RX2]) ;

}

void instancelogrxdata(instance_data_t *inst, uint8 *buffer, int length)
{
	int i = 0;
	int j = msgbufferidx; //latest entry

	msgbufferlen[j] = length;
	memset(&msgbuffer[msgbufferidx][length], 0, (STAT_LINE_MSGBUF_LENGTH - length)); //clear old data
	memcpy(&msgbuffer[msgbufferidx][0], buffer, length); //write new data
	
	msgbufferidx++;
	if(msgbufferidx == NUM_MSGBUFF_LINES) //check wrap
			msgbufferidx = 0;
	

    //copy data to screen buffer for displaying
    
    copybufftoscrline(&inst->statusrep.lmsg_scrbuf[j][0], (char*) &msgbuffer[j][0], msgbufferlen[j]);

    inst->statusrep.lmsg_last_index = j ; // tell instance what is last (newest) message written

}


void instancecalculatepower(void)
{
	int instance = 0;
	int i = 0;

	double alpha = (instance_data[instance].configData.rxCode > 8)? (-115.72-6.02) : (-115.72); 
	
	double F1 = instance_data[instance].dwlogdata.diag.firstPathAmp1 * instance_data[instance].dwlogdata.diag.firstPathAmp1;
	double F2 = instance_data[instance].dwlogdata.diag.firstPathAmp2 * instance_data[instance].dwlogdata.diag.firstPathAmp2;
	double F3 = instance_data[instance].dwlogdata.diag.firstPathAmp3 * instance_data[instance].dwlogdata.diag.firstPathAmp3;

	double N = instance_data[instance].dwlogdata.diag.rxPreamCount * instance_data[instance].dwlogdata.diag.rxPreamCount;

	instance_data[instance].RSL = 10*log10((double)(instance_data[instance].dwlogdata.diag.maxGrowthCIR)/N)+alpha+51.175;
	            
	instance_data[instance].FSL = 10*log10((double)((F1+F2+F3)/N))+alpha;

}

// -------------------------------------------------------------------------------------------------------------------
// function to allow top level access to accumulator data (e.g. for graphing)
//
const char * instancegetaccumulatordata(accumPlotInfo_t *ap)
{

#if (DECA_SUPPORT_SOUNDING==1) && (DECA_KEEP_ACCUMULATOR==1)
    int instance = 0;
    int i ;
	double delta;
	
    // calculate magnitude values
    for (i = 0 ; i < MAX_ACCUMULATOR_ELEMENTS ; i++)
    {
        ap->magvals[i] = (int32) 
			(sqrt((double) (instance_data[instance].dwacclogdata.accumCmplx[i].real * instance_data[instance].dwacclogdata.accumCmplx[i].real 
			+ instance_data[instance].dwacclogdata.accumCmplx[i].imag * instance_data[instance].dwacclogdata.accumCmplx[i].imag))) ;
    }

    for(i=0; i< MAX_ACCUMULATOR_ELEMENTS; i++)
    {
        ap->accvals[i] = instance_data[instance].dwacclogdata.accumCmplx[i] ;     // copy accumulator data.
    }
    ap->accumSize = instance_data[instance].dwacclogdata.accumLength ;       // sets size of accumulator data
    ap->windInd = 735; //instance_data[instance].phydata.dd.accumSfdWindowIndex ;
	
	ap->hwrFirstPath = instance_data[instance].dwlogdata.diag.firstPath; 
	ap->maxGrowthCIR = instance_data[instance].dwlogdata.diag.maxGrowthCIR;
    ap->preambleSymSeen = instance_data[instance].dwlogdata.diag.rxPreamCount ;	
	ap->RSL = instance_data[instance].RSL;

	delta = 87-7.5 ;

	if((instance_data[instance].configData.chan == 4) || (instance_data[instance].configData.chan == 7)) //900 MHz
		delta -= 2.5;

    ap->totalSNR = ap->RSL + delta;
	
	arrRSL[index] = ap->RSL ;
	arrSNR[index++] = ap->totalSNR ;

	if(index == 10) index = 0;

	ap->totalSNRavg = 0;
	for(i = 0; i < 10; i++)
    {
        ap->totalSNRavg += arrSNR[i];
    }
    ap->totalSNRavg /= 10;
	ap->RSLavg = 0;
	for(i = 0; i < 10; i++)
    {
        ap->RSLavg += arrRSL[i];
    }
    ap->RSLavg /= 10;

	if(instance_data[instance].rxmsgcount < 10)
	{
		ap->totalSNRavg = 0;
		ap->RSLavg = 0;
	}


    ap->fpSNR = -99.0f;
    ap->fpRelAtten = -99.0f;

	switch (instance_data[instance].dwacclogdata.erroredFrame)
    {
    case DWT_SIG_RX_NOERR     : return "OKAY   " ;                  // NB: good one must begin with 'O' and no others should !!
    case DWT_SIG_RX_ERROR     : return "CRC ERR" ;
    case DWT_SIG_RX_SYNCLOSS  : return "RS SYNC" ;
    case DWT_SIG_RX_PHR_ERROR : return "PHR ERR" ;
	case DWT_SIG_RX_PTOTIMEOUT : return "PTO TMO" ;
    case DWT_SIG_RX_SFDTIMEOUT : return "SFD TMO" ;
    default               : return "UNEXPCT" ;
    }
#else
    ap->accvals = NULL ;
    ap->accumSize = 0 ;
    //ap->magvals = NULL ;
    ap->windInd = 0 ;
    ap->hwrFirstPath = -1.0 ;
    ap->preambleSymSeen = 0 ;
    return "EMPTY!!" ;
#endif
}

// -------------------------------------------------------------------------------------------------------------------
//
// Is there new plot data to display
//
int instancenewplotdata(void)
{
    int instance = 0;
#if (DECA_SUPPORT_SOUNDING==1)
#if DECA_KEEP_ACCUMULATOR==1
	if (instance_data[instance].dwacclogdata.newAccumData)
    {
        // flag seen/calculated
        instance_data[instance].dwacclogdata.newAccumData = 0 ;
        return 1 ;
	}
#endif
#endif
	return 0;
}

int instancenewrangedata(void)
{
	int x = new_range;
	new_range = 0;

	return x;
}

double instancenewrange(void)
{
	int instance = 0;

	return instance_data[instance].idistance;
}

// -------------------------------------------------------------------------------------------------------------------
statusReport_t * getstatusreport(void)
{
    // assume instance 0, for this
    return &instance_data[0].statusrep ;

}


// -------------------------------------------------------------------------------------------------------------------

FILE* instanceaccumloghandle(void)
{
#if (DECA_LOG_ENABLE==1) || (DECA_ACCUM_LOG_SUPPORT==1)
    int instance = 0 ;

	return instance_data[instance].dwlogdata.accumLogFile;
#else
    FILE* fp = NULL;

    return fp;
#endif
}

// -------------------------------------------------------------------------------------------------------------------
//
// low level function to process accumulator data - convert to real/imaginary and calculate magnitude
//
//
#if DECA_SUPPORT_SOUNDING==1
void processSoundingData(void)
{
	int instance = 0;

	if (isbigendian())              // high byte should come first, but device reads it low byte first
    {
        int a, b ;
        for (a = 0, b = 0 ; a < instance_data[instance].dwacclogdata.accumLength ; a++)
        {
            instance_data[instance].dwacclogdata.accumCmplx[a].real = instance_data[instance].dwacclogdata.buff.accumData->accumRawData[b] + (instance_data[instance].dwacclogdata.buff.accumData->accumRawData[b+1] << 8) ;
            instance_data[instance].dwacclogdata.accumCmplx[a].imag = instance_data[instance].dwacclogdata.buff.accumData->accumRawData[b+2] + (instance_data[instance].dwacclogdata.buff.accumData->accumRawData[b+3] << 8) ;
            b += 4 ;
        }
    }
    else                            // read order matches int16 order for this processor
    {
        memcpy(instance_data[instance].dwacclogdata.accumCmplx,instance_data[instance].dwacclogdata.buff.accumData->accumRawData,instance_data[instance].dwacclogdata.buff.accumLength*sizeof(complex16)) ;
    }

    return ;

} // end processaccumulator()
#endif

extern void _dwt_enableclocks(int clocks) ;

void print_debugLDE(void)
{
#if DECA_SUPPORT_SOUNDING==1
	int instance = 0;
	int x;
	uint32 ldedbg = 0;
	uint16 ldedbg1 = 0;
	uint16 ldedbg2 = 0;
	uint16 ldedbg3 = 0;
	uint16 ldedbg4 = 0;
	int ldedbgint1,  ldedbgint2;
	int ldedbga[17];
	{
		//
		//dump raw LDE data
		ldedbg = dwt_read32bitoffsetreg(0x2e, 0x0);
		ldedbga[0] = ldedbgint1 = (ldedbg & 0xFFFF);
		ldedbga[1] = ldedbgint2 = ((ldedbg >> 16) & 0xFFFF);
		//fprintf(instance_data[instance].devicelogdata.accumLogFile,"%04x %d\n%04x %d\n", ldedbg & 0xFFFF, ldedbgint1, (ldedbg >> 16) & 0xFFFF, ldedbgint2);
		ldedbg = dwt_read32bitoffsetreg(0x2e, 0x4);
		ldedbga[2] = ldedbgint1 = (ldedbg & 0xFFFF);
		ldedbga[3] = ldedbgint2 = ((ldedbg >> 16) & 0xFFFF);
		//fprintf(instance_data[instance].devicelogdata.accumLogFile,"%04x %d\n%04x %d\n", ldedbg & 0xFFFF, ldedbgint1, (ldedbg >> 16) & 0xFFFF, ldedbgint2);
		ldedbg = dwt_read32bitoffsetreg(0x2e, 0x1000);
		ldedbga[4] = ldedbgint1 = (ldedbg & 0xFFFF);
		ldedbga[5] = ldedbgint2 = ((ldedbg >> 16) & 0xFFFF);
		//fprintf(instance_data[instance].devicelogdata.accumLogFile,"%04x %d\n%04x %d\n", ldedbg & 0xFFFF, ldedbgint1, (ldedbg >> 16) & 0xFFFF, ldedbgint2);
		ldedbg = dwt_read32bitoffsetreg(0x2e, 0x2004);
		ldedbga[6] = ldedbgint1 = (ldedbg & 0xFFFF);
		ldedbga[7] = ldedbgint2 = ((ldedbg >> 16) & 0xFFFF);
		//fprintf(instance_data[instance].devicelogdata.accumLogFile,"%04x %d\n%04x %d\n", ldedbg & 0xFFFF, ldedbgint1, (ldedbg >> 16) & 0xFFFF, ldedbgint2);
		ldedbg = dwt_read32bitoffsetreg(0x2e, 0x2000);
		ldedbga[8] = ldedbgint1 = (ldedbg & 0xFFFF);
		ldedbga[9] = ldedbgint2 = ((ldedbg >> 16) & 0xFFFF);
		//fprintf(instance_data[instance].devicelogdata.accumLogFile,"%04x %d\n%04x %d\n", ldedbg & 0xFFFF, ldedbgint1, (ldedbg >> 16) & 0xFFFF, ldedbgint2);
		ldedbg = dwt_read32bitoffsetreg(0x2e, 0x2008);
		ldedbga[10] = ldedbgint1 = (ldedbg & 0xFFFF);
		ldedbga[11] = ldedbgint2 = ((ldedbg >> 16) & 0xFFFF);
		//fprintf(instance_data[instance].devicelogdata.accumLogFile,"%04x %d\n%04x %d\n", ldedbg & 0xFFFF, ldedbgint1, (ldedbg >> 16) & 0xFFFF, ldedbgint2);
		ldedbg = dwt_read32bitoffsetreg(0x2e, 0x100A);
		ldedbga[12] = ldedbgint1 = (ldedbg & 0xFFFF);
		ldedbga[13] = ldedbgint2 = ((ldedbg >> 16) & 0xFFFF);
		//fprintf(instance_data[instance].devicelogdata.accumLogFile,"%04x %d\n%04x %d\n", ldedbg & 0xFFFF, ldedbgint1, (ldedbg >> 16) & 0xFFFF, ldedbgint2);				
		ldedbg = dwt_read32bitoffsetreg(0x2e, 0x1004);
		ldedbga[14] = ldedbgint1 = (ldedbg & 0xFFFF);
		ldedbga[15] = ldedbgint2 = ((ldedbg >> 16) & 0xFFFF);
		//fprintf(instance_data[instance].devicelogdata.accumLogFile,"%04x %d\n%04x %d\n", ldedbg & 0xFFFF, ldedbgint1, (ldedbg >> 16) & 0xFFFF, ldedbgint2);					
		ldedbg = dwt_read32bitoffsetreg(0x2e, 0x1008);
		ldedbga[16] = ldedbgint1 = (ldedbg & 0xFFFF);
		//fprintf(instance_data[instance].devicelogdata.accumLogFile,"%04x %d\n", ldedbg & 0xFFFF, ldedbgint1);
	}

	fprintf(instance_data[instance].dwlogdata.accumLogFile,"\nDBG LDE ");
	for(x=0; x < 17; x++)
		fprintf(instance_data[instance].dwlogdata.accumLogFile,"dbg%d(%04X), ", x+1, ldedbga[x]);
	fprintf(instance_data[instance].dwlogdata.accumLogFile,"\n");
	ldedbg4 = dwt_read16bitoffsetreg(0x2e, 0x2804);
	ldedbg3 = dwt_read16bitoffsetreg(0x2e, 0x1806);
	ldedbg1 = dwt_read16bitoffsetreg(0x2e, 0x806);
	ldedbg2 = dwt_read16bitoffsetreg(0x2e, 0x804);
	fprintf(instance_data[instance].dwlogdata.accumLogFile,"PARAM LDE %04x %04x %04x\n", ldedbg2, ldedbg1, ldedbg3, ldedbg4);
#endif
}

void print_rxmessage(event_data_t *dw_event)
{

#if(DECA_LOG_ENABLE==1)
	int instance = 0;
	int x;
	uint8 *rxmsgx = (uint8*)&dw_event->msgu.frame[0];
	fprintf(instance_data[instance].dwlogdata.accumLogFile,"RX DATA: ");
	for(x=0; x<dw_event->rxLength; x++)
	{
		fprintf(instance_data[instance].dwlogdata.accumLogFile,"%02x", rxmsgx[x]);
	}
	fprintf(instance_data[instance].dwlogdata.accumLogFile,"\n");
#endif
}


#if DECA_ACCUM_LOG_SUPPORT==1 || DECA_LOG_ENABLE==1
const char *errorType = "xxxCxHRxxx" ;  // indicate Crc error SIG_RX_ERROR(==3), SIG_RX_PHR_ERROR(==5) or SIG_RX_SYNCLOSS(==6)
#endif

void logSoundingData(int8 errorFlag, int fc, int sn, event_data_t *dw_event)
{
	int instance = 0;
	uint8	rsmpdel = 0;
	uint16 tempvbat;
#if (DECA_ACCUM_LOG_SUPPORT==1)
	int len = instance_data[instance].dwacclogdata.accumLength = instance_data[instance].dwacclogdata.buff.accumLength;
	//if (!buff->rxi.ranging) len = 0; //if not a ranging frame we don't read the accumulator
#endif
#if DECA_LOG_ENABLE==1
#if (DECA_ACCUM_LOG_SUPPORT==1)
	deca_buffer_t *buff = &instance_data[instance].dwacclogdata.buff;
#endif

	//	Note on Temperature: the temperature value needs to be converted to give the real temperature
	//  the formula is: 1.13 * reading - 113.0
	//  Note on Voltage: the voltage value needs to be converted to give the real voltage
	//  the formula is: 0.0057 * reading + 2.3
	
	//NOTE - if RX needs to be on when reading temperature then use tempvbat = dwt_readtempvbat(1);	- use fast SPI... and DW1000 is not in IDLE
	//if want to read the temp with slow SPI, and DW1000 is in IDLE then need to use tempvbat = dwt_readtempvbat(1);
	/*{
		instancesetspibitrate(DW_DEFAULT_SPI);

		tempvbat = dwt_readtempvbat(0);	

		instancesetspibitrate(DW_FAST_SPI);

		if(instance_data[instance].rxautoreenable == 1)
			instancerxon(&instance_data[instance], 0, 0); //immediate enable if anchor or listener
	}*/
	tempvbat = dwt_readtempvbat(1);	
	//printf("%04x\n", tempvbat);

	dwt_readfromdevice(0x14 /*RX_TTCKO_ID*/, 3, 1, &rsmpdel); //RESAMPLING DELAY

    switch (instance_data[instance].dwlogdata.accumLogging)
    {
    case LOG_ERR_ACCUM :

        if (!errorFlag) break ;  // in the errored case we don't log normal accumulators

	case LOG_ALL_NOACCUM:
    case LOG_ALL_ACCUM :
        {
            int i = 0, x = 0;
			//uint8 temp[1];
			uint16 rxPC = 0;
			double lpath = 0;
            switch (errorFlag)
            {
            case DWT_SIG_RX_NOERR :
				{
					uint16 rxunadj_hi16 = dwt_read16bitoffsetreg(0x15, 13) & 0xFF;
					uint32 rxunadj_low32 = dwt_read32bitoffsetreg(0x15, 9);
					uint16 rxadj_hi16 = (dw_event->timeStamp >> 32) & 0xFF;
					uint32 rxadj_low32 = (dw_event->timeStamp & 0xFFFFFFFF);
					uint64 rxUnTimeStamp = rxunadj_hi16;
					rxUnTimeStamp <<= 32;
					rxUnTimeStamp += rxunadj_low32;

					fprintf(instance_data[instance].dwlogdata.accumLogFile,"\n%02X %02X Rx time = %4.15e %02X%08X\n", fc /*FC*/, sn /*SN*/, 
																				convertdevicetimetosecu(dw_event->timeStamp), rxadj_hi16, rxadj_low32) ; // DEBUG
					fprintf(instance_data[instance].dwlogdata.accumLogFile,"\n%02X %02X Rx time(un) = %4.15e %02X%08X\n", fc /*FC*/, sn /*SN*/, 
																				convertdevicetimetosecu(rxUnTimeStamp), rxunadj_hi16, rxunadj_low32) ; // DEBUG
					fprintf(instance_data[instance].dwlogdata.accumLogFile,"\ntxdly %04x rxdly %04x\n", dwt_read16bitoffsetreg(0x18, 0x0), dwt_read16bitoffsetreg(0x2e, 0x1804));
					//print_debugLDE();
					
					print_rxmessage(dw_event);
					
					fprintf(instance_data[instance].dwlogdata.accumLogFile,"\nRX OK WInd(0735), HLP(%09.4f), PSC(%04i), SLP(%09.4f), RC(%04X %08X),",
						instance_data[instance].dwlogdata.diag.firstPath, instance_data[instance].dwlogdata.diag.rxPreamCount,  lpath,
						rxadj_hi16, rxadj_low32);
					fprintf(instance_data[instance].dwlogdata.accumLogFile," DCR(0), DCI(0), NTH(%04X), T(%04X), RSL(%09.4f), FSL(%09.4f), RSMPL(%02X)",
						instance_data[instance].dwlogdata.diag.maxNoise, tempvbat, instance_data[instance].RSL, instance_data[instance].FSL, rsmpdel);
//					fprintf(instance_data[instance].devicelogdata.accumLogFile," DCR(0), DCI(0), NTH(%04X), T(%04X), RSL(%09.4f), FSL(%09.4f), CI(%08x), TI(%08x)",
//						instance_data[instance].devicelogdata.diag.maxNoise, tempvbat, instance_data[instance].RSL, instance_data[instance].FSL, instance_data[instance].devicelogdata.diag.debug1, instance_data[instance].devicelogdata.diag.debug2);
				
				}
				break ;
#if DECA_ERROR_LOGGING==1
            case DWT_SIG_RX_ERROR :
            case DWT_SIG_RX_PHR_ERROR :
            case DWT_SIG_RX_SYNCLOSS :
			case DWT_SIG_RX_PTOTIMEOUT :
            case DWT_SIG_RX_SFDTIMEOUT :
				{			
					dwt_deviceentcnts_t itemp = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
					instancereadevents(&itemp);

					fprintf(instance_data[instance].dwlogdata.accumLogFile,"\n");
					fprintf(instance_data[instance].dwlogdata.accumLogFile,"\nRX E%c WInd(0735), PSC(%04i), CE(%04i), RSE(%04i), HE(%04i), STO(%04i), FFE(%04i), T(%04X), RSL(%09.4f), FSL(%09.4f), RSMPL(%02X)",
                        errorType[errorFlag], // single character from char array defined above
                        instance_data[instance].dwlogdata.diag.rxPreamCount,  
                        itemp.CRCB, itemp.RSL, itemp.PHE, itemp.SFDTO, itemp.ARFE, tempvbat, instance_data[instance].RSL, instance_data[instance].FSL, rsmpdel) ;
				}
                break ;
#endif
            }
#if DECA_ACCUM_LOG_SUPPORT==1
			if(instance_data[instance].dwlogdata.accumLogging != LOG_ALL_NOACCUM)
            {
				fprintf(instance_data[instance].dwlogdata.accumLogFile,"\nAccum Len %i\n",len) ;
				for (i = 0 ; i < len ; i++)
				fprintf(instance_data[instance].dwlogdata.accumLogFile,"%i, %i\n",instance_data[instance].dwacclogdata.accumCmplx[i].real, instance_data[instance].dwacclogdata.accumCmplx[i].imag) ;
			
				/*fprintf(instance_data[instance].dwlogdata.accumLogFile,"\nPreamb Len %i\n",1024) ;
				for (i = 0 ; i < 1024 ; i++)
				{
					fprintf(instance_data[instance].dwlogdata.accumLogFile,"%d, %d\n", i, instance_data[instance].dwacclogdata.preambData[i]) ;
				}*/


			}

#endif
			fflush(instance_data[instance].dwlogdata.accumLogFile) ;

        } // end logging all

    } // end logging enabled

#endif // DECA_ACCUM_LOG_SUPPORT
}




// -------------------------------------------------------------------------------------------------------------------
//
// Functions relating to logging accumulator data
//
//#if (DECA_SUPPORT_SOUNDING==1)
#if DECA_ACCUM_LOG_SUPPORT==1 || DECA_LOG_ENABLE==1

#define DECA_ALLACCUM_LOG_FILE      "_DecaWaveAllAccum.log"   // file name (tail) to use, date string is prefixed
#define DECA_ERRACCUM_LOG_FILE      "_DecaWaveErrAccum.log"   // file name (tail) to use, date string is prefixed
#define DECA_ACCUM_LOG_FILE			"_DecaWaveAccum.log"	  // file name (tail) to use, date string is prefixed

#include "DecaRanging_Ver.h"
char * build = __DATE__  ", " __TIME__ ;

int _instanceaccumlogenable(int logEnable)
{
	int instance = 0;
    char fnbuf[sizeof(DECA_ERRACCUM_LOG_FILE)+DECA_STAMP_STRING_LENGTH] ;   // declare buffer for file name
    char *s ;

    // close any active log  -- might call this from decaphyclose below tbd
    if (instance_data[instance].dwlogdata.accumLogging != 0)
    {
         fclose(instance_data[instance].dwlogdata.accumLogFile) ;
         instance_data[instance].dwlogdata.accumLogging = 0 ;
         instance_data[instance].dwlogdata.accumLogFile = NULL ;
    }

    if (logEnable == LOG_ACCUM_OFF) return 0 ;      // disabling logging, nothing else to do

    // Open new log file

    s = filldatetimestring(fnbuf,sizeof(fnbuf)) ;       // begin filename with date code and ...

    if ((logEnable == LOG_ALL_ACCUM) || (logEnable == LOG_ALL_NOACCUM))                 // ...then tag on appropriate file name end
    {
        strcpy_s(s,sizeof(fnbuf)-(s-fnbuf),DECA_ALLACCUM_LOG_FILE) ;
    }
	else if(logEnable == LOG_SINGLE_ACC)
	{
		 strcpy_s(s,sizeof(fnbuf)-(s-fnbuf),DECA_ACCUM_LOG_FILE) ;
	}
    else
    {
        strcpy_s(s,sizeof(fnbuf)-(s-fnbuf),DECA_ERRACCUM_LOG_FILE) ;
    }

    if ((instance_data[instance].dwlogdata.accumLogFile = _fsopen(fnbuf,"wt",_SH_DENYWR))==NULL)  // open the file (allow reading by text editor)
    {
        printf("\n\aERROR - LOG FILE OPEN FAILED.\a\n\n") ;
        instance_data[instance].dwlogdata.accumLogging = 0 ;
        return 1 ; // 1 means no file opened
    }

    fprintf(instance_data[instance].dwlogdata.accumLogFile,"File:%s, created by %s (build:%s)\n",fnbuf,SOFTWARE_VER_STRING,build);

    instance_data[instance].dwlogdata.accumLogging = logEnable ;

    return 0 ; // means success

} // end instanceaccumlogenable()


// allow upper layers to add a string line to phy log file
void instancestatusaccumlog(int instance, const char *tag, const char *line)
{
    if ((instance_data[instance].dwlogdata.accumLogging == LOG_ALL_ACCUM) || (instance_data[instance].dwlogdata.accumLogging == LOG_ALL_NOACCUM))
    {
        fprintf(instance_data[instance].dwlogdata.accumLogFile,"\n[%s] %s\n", tag, line) ;
    }
} // end instancestatusaccumlog()

#endif
//#endif


// -------------------------------------------------------------------------------------------------------------------
//
// Returns 0 for success, 1 for failure
//
#define SELECTED_PREAM_LENGTHS	(8)
uint16 plengths[SELECTED_PREAM_LENGTHS] = { 64, 128, 256, 512, 1024, 1536, 2048, 4096 };
uint8 plengthcodes[SELECTED_PREAM_LENGTHS] = { DWT_PLEN_64, DWT_PLEN_128, DWT_PLEN_256, DWT_PLEN_512, DWT_PLEN_1024, DWT_PLEN_1536, DWT_PLEN_2048, DWT_PLEN_4096 };

int instanceaccumlogenable(int logEnable)
{
#if DECA_ACCUM_LOG_SUPPORT==1 || DECA_LOG_ENABLE==1

    int instance = 0 ;
	int success = 1;

	uint8 chan;
	uint8 code;
	uint8 pac;
	uint8 prf;
	uint16 plen;

    switch (logEnable)
    {
    case INST_LOG_OFF : success = _instanceaccumlogenable(0) ; break;
    case INST_LOG_ERR : success = _instanceaccumlogenable(LOG_ERR_ACCUM) ; break;
    case INST_LOG_ALL_NOACC : success = _instanceaccumlogenable(LOG_ALL_NOACCUM) ; break;
    case INST_LOG_SINGLE : success = _instanceaccumlogenable(LOG_SINGLE_ACC) ; break;
    case INST_LOG_ALL : success = _instanceaccumlogenable(LOG_ALL_ACCUM) ; break;
    }

	if(logEnable != INST_LOG_OFF)
	{
		uint32 lde_ver = 0;
		uint16 antenna_delay = dwt_readantennadelay(instance_data[instance].configData.prf);
		int i = 0;

		for(i=0; i<SELECTED_PREAM_LENGTHS; i++)
		{
			if(plengthcodes[i] == instance_data[instance].configData.txPreambLength)
				plen = plengths[i] ;
		}

		chan = instance_data[instance].configData.chan;

		code = instance_data[instance].configData.rxCode;

		pac = instance_data[instance].configData.rxPAC;

		prf = ((instance_data[instance].configData.prf == DWT_PRF_16M) ? 16 : 64) ;

		//Add instance channel config data to the log...
		fprintf(instance_data[instance].dwlogdata.accumLogFile,"Mode: %d, Chan %d, Code %d, PRF %d, Plength %d, DataRate %d, PAC %d, ic:%04x, ucode:xxxx, antdl:%04X\n", 
																		instance_data[instance].mode, chan, code,
																		prf,
																		plen,
																		(instance_data[instance].configData.dataRate+1),
																		pac, instance_data[instance].dwlogdata.icid,
																		antenna_delay);
		fflush(instance_data[instance].dwlogdata.accumLogFile);
	}
    return success;
#else
       return 1 ; // should not get here
#endif
}

// -------------------------------------------------------------------------------------------------------------------
//
// Returns 0 for success, 1 for failure
//
void instanceaccumlogflush(void)
{

#if DECA_ACCUM_LOG_SUPPORT==1 || DECA_LOG_ENABLE==1
    int instance = 0 ;

    if (instance_data[instance].dwlogdata.accumLogFile) fflush(instance_data[instance].dwlogdata.accumLogFile) ;
#endif

    return; // should not get here
}




/* ==========================================================

Notes:

Previously code handled multiple instances in a single console application

Now have changed it to do a single instance only. With minimal code changes...(i.e. kept [instance] index but it is always 0.

Windows application should call instance_init() once and then in the "main loop" call instance_run().

*/
