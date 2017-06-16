#include "envelope_detector.h"
#include "defjams.h"
#include <math.h>

//typedef struct envelope_detector
//{
//    int  m_sample;
//    double m_attacktime;
//    double m_releasetime;
//    double m_attacktime_msec;
//    double m_releasetime_msec;
//    double m_envelope;
//    unsigned int  m_detectmode; // PEAK, MS, RMS
//    bool  m_analogtc;
//    bool  m_logdetector;
//} envelope_detector;

void envelope_detector_init(envelope_detector *ed, double attack_ms, double release_ms, bool analogtc, unsigned int detect, bool logdetector)
{
    ed->m_sample = 0;
    ed->m_envelope = 0.;
    ed->m_attacktime_ms = attack_ms; 
    ed->m_releasetime_ms = release_ms; 
    ed->m_detectmode = detect;
    ed->m_analogtc = analogtc;
    ed->m_logdetector = logdetector;
    envelope_detector_setattacktime(ed, attack_ms);
    envelope_detector_setreleasetime(ed, release_ms);
}

void envelope_detector_setattacktime(envelope_detector *ed, double attack_ms)
{
	ed->m_attacktime_ms = attack_ms;
    if(ed->m_analogtc)
        ed->m_attacktime = exp(ANALOG_TC/( attack_ms * SAMPLE_RATE * 0.001));
    else
        ed->m_attacktime = exp(DIGITAL_TC/( attack_ms * SAMPLE_RATE * 0.001));
}
void envelope_detector_setreleasetime(envelope_detector *ed, double release_ms);
double envelope_detector_detect(envelope_detector *ed, double input);
