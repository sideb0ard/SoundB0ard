#include "envelope_detector.h"
#include "defjams.h"
#include <math.h>

const float DIGITAL_TC = -2.0;                               // log(1%)
const float ANALOG_TC = -0.43533393574791066201247090699309; // (log(36.7%)

void envelope_detector_init(envelope_detector *ed, double attack_ms,
                            double release_ms, bool analogtc,
                            unsigned int detect, bool logdetector)
{
    ed->m_sample = 0;
    ed->m_envelope = 0.;
    ed->m_attack_ms = attack_ms;
    ed->m_release_ms = release_ms;
    ed->m_detectmode = detect;
    ed->m_analogtc = analogtc;
    ed->m_logdetector = logdetector;
    envelope_detector_setattacktime(ed, attack_ms);
    envelope_detector_setreleasetime(ed, release_ms);
}

void envelope_detector_setattacktime(envelope_detector *ed, double attack_ms)
{
    ed->m_attack_ms = attack_ms;
    if (ed->m_analogtc)
        ed->m_attacktime = exp(ANALOG_TC / (attack_ms * SAMPLE_RATE * 0.001));
    else
        ed->m_attacktime = exp(DIGITAL_TC / (attack_ms * SAMPLE_RATE * 0.001));
}

void envelope_detector_setreleasetime(envelope_detector *ed, double release_ms)
{
    ed->m_release_ms = release_ms;
    if (ed->m_analogtc)
        ed->m_releasetime = exp(ANALOG_TC / (release_ms * SAMPLE_RATE * 0.001));
    else
        ed->m_releasetime =
            exp(DIGITAL_TC / (release_ms * SAMPLE_RATE * 0.001));
}

void envelope_detector_settcmodeanalog(envelope_detector *ed, bool analogtc)
{
    ed->m_analogtc = analogtc;
    envelope_detector_setattacktime(ed, ed->m_attack_ms);
    envelope_detector_setreleasetime(ed, ed->m_release_ms);
}

double envelope_detector_detect(envelope_detector *ed, double input)
{
    switch (ed->m_detectmode) {
    case 0:
        input = fabs(input);
    case 1:
        input = fabs(input) * fabs(input);
    case 2:
        input = pow((double)fabs(input) * (double)fabs(input), (double)0.5);
    default:
        input = fabs(input);
    }

    if (input > ed->m_envelope)
        ed->m_envelope = ed->m_attacktime * (ed->m_envelope - input) + input;
    else
        ed->m_envelope = ed->m_releasetime * (ed->m_envelope - input) + input;

    if (ed->m_envelope > 0.0 && ed->m_envelope < FLT_MIN_PLUS)
        ed->m_envelope = 0.;
    if (ed->m_envelope < 0.0 && ed->m_envelope > FLT_MIN_MINUS)
        ed->m_envelope = 0.;

    ed->m_envelope = min(ed->m_envelope, 1.0);
    ed->m_envelope = max(ed->m_envelope, 0.0);

    if (ed->m_logdetector) {
        if (ed->m_envelope <= 0)
            return -96.0;
        return 20 * log10(ed->m_envelope);
    }
    return ed->m_envelope;
}

void envelope_detector_setdetectmode(envelope_detector *ed, unsigned int detect)
{
    ed->m_detectmode = detect;
}
void envelope_detector_setlogdetect(envelope_detector *ed, bool b)
{
    ed->m_logdetector = b;
}
