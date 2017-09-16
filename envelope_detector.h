#pragma once

#include <stdbool.h>

// constants for dealing with overflow or underflow
#define FLT_EPSILON_PLUS                                                       \
    1.192092896e-07 /* smallest such that 1.0+FLT_EPSILON != 1.0 */
#define FLT_EPSILON_MINUS                                                      \
    -1.192092896e-07 /* smallest such that 1.0-FLT_EPSILON != 1.0 */
#define FLT_MIN_PLUS 1.175494351e-38   /* min positive value */
#define FLT_MIN_MINUS -1.175494351e-38 /* min negative value */

#ifndef max
#define max(a, b) (((a) > (b)) ? (a) : (b))
#endif
#ifndef min
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif

enum
{
    PEAK,
    MS,
    RMS
};
enum
{
    DETECT_MODE_PEAK,
    DETECT_MODE_MS,
    DETECT_MODE_RMS,
    DETECT_MODE_NONE
};

typedef struct envelope_detector
{
    int m_sample;
    double m_envelope;

    double m_attacktime;
    double m_releasetime;
    double m_attack_ms;
    double m_release_ms;

    unsigned int m_detectmode; // PEAK, MS, RMS

    bool m_analogtc;
    bool m_logdetector;

} envelope_detector;

void envelope_detector_init(envelope_detector *ed, double attack_ms,
                            double release_ms, bool analogtc,
                            unsigned int detect, bool logdetector);

void envelope_detector_settcmodeanalog(envelope_detector *ed, bool analogtc);

void envelope_detector_setdetectmode(envelope_detector *ed,
                                     unsigned int detect);
void envelope_detector_setlogdetect(envelope_detector *ed, bool b);

void envelope_detector_setattacktime(envelope_detector *ed, double attack_ms);
void envelope_detector_setreleasetime(envelope_detector *ed, double release_ms);

double envelope_detector_detect(envelope_detector *ed, double input);
