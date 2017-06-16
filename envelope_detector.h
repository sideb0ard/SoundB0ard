#pragma once
#include <stdbool.h>

enum {PEAK, MS, RMS};
const float DIGITAL_TC = -2.0; // log(1%)
const float ANALOG_TC = -0.43533393574791066201247090699309; // (log(36.7%)

typedef struct envelope_detector
{
    int  m_sample;
    double m_envelope;

    double m_attacktime;
    double m_releasetime;
    double m_attacktime_ms;
    double m_releasetime_ms;

    unsigned int  m_detectmode; // PEAK, MS, RMS

    bool  m_analogtc;
    bool  m_logdetector;

} envelope_detector;

void envelope_detector_init(envelope_detector *ed, double attack_ms, double release_ms, bool analogtc, unsigned int detect, bool logdetector);
void envelope_detector_settcmodeanalog(envelope_detector *ed, bool analogtc) {ed->m_analogtc = analogtc;}
void envelope_detector_setdetectmode(envelope_detector *ed, unsigned int detect) {ed->m_detectmode = detect;}
void envelope_detector_setlogdetect(envelope_detector *ed, bool b) {ed->m_logdetector = b;}

void envelope_detector_setattacktime(envelope_detector *ed, double attack_ms);
void envelope_detector_setreleasetime(envelope_detector *ed, double release_ms);

double envelope_detector_detect(envelope_detector *ed, double input);
