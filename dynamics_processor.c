#include <stdlib.h>

#include "dynamics_processor.h"
#include "utils.h"

const char *dynamics_processor_type_to_char[] = {"COMP", "LIMIT", "EXPAND",
                                                 "GATE"};

dynamics_processor *new_dynamics_processor(void)
{
    dynamics_processor *dp = calloc(1, sizeof(dynamics_processor));
    dp->m_fx.type = COMPRESSOR;
    dp->m_fx.enabled = true;
    dp->m_fx.process = &dynamics_processor_process;
    dp->m_fx.status = &dynamics_processor_status;

    dp->m_inputgain_db = 0;      // -12 - 20
    dp->m_threshold = 0;         // -60 - 0
    dp->m_attack_ms = 20;        // 1 - 300
    dp->m_release_ms = 1000;     // 20 - 5000
    dp->m_ratio = 1;             // 1 - 20
    dp->m_outputgain_db = 0;     // 0 - 20
    dp->m_knee_width = 0;        // 0 - 20
    dp->m_processor_type = COMP; // 0-3 COMP, LIMIT, EXPAND GATE
    dp->m_time_constant = 0;     // digital, analog

    dynamics_processor_init(dp);

    return dp;
}
void dynamics_processor_init(dynamics_processor *dp)
{
    if (dp->m_time_constant == 1) // digital
    {
        envelope_detector_init(&dp->m_left_detector, dp->m_attack_ms,
                               dp->m_release_ms, false, DETECT_MODE_RMS, true);
        envelope_detector_init(&dp->m_right_detector, dp->m_attack_ms,
                               dp->m_release_ms, false, DETECT_MODE_RMS, true);
    }
    else {
        envelope_detector_init(&dp->m_left_detector, dp->m_attack_ms,
                               dp->m_release_ms, true, DETECT_MODE_RMS, true);
        envelope_detector_init(&dp->m_right_detector, dp->m_attack_ms,
                               dp->m_release_ms, true, DETECT_MODE_RMS, true);
    }
    delay_init(&dp->m_left_delay, 300);
    delay_init(&dp->m_right_delay, 300);
    delay_reset_delay(&dp->m_left_delay);
    delay_reset_delay(&dp->m_right_delay);
}

double dynamics_processor_calc_compression_gain(double detector_val,
                                                double threshold, double rratio,
                                                double kneewidth, bool limit)
{
    double cs = 1.0 - 1.0 / rratio;
    if (limit)
        cs = 1;

    if (kneewidth > 0 && detector_val > (threshold - kneewidth / 2.0) &&
        detector_val < threshold + kneewidth / 2.0) {
        double x[2];
        double y[2];
        x[0] = threshold - kneewidth / 2.0;
        x[1] = threshold + kneewidth / 2.0;
        x[1] = min(0, x[1]);
        y[0] = 0;
        y[1] = cs;
        cs = lagrpol(&x[0], &y[0], 2, detector_val);
    }
    double yg = cs * (threshold = detector_val);
    yg = min(0, yg);
    return pow(10.0, yg / 20.0);
}

double dynamics_processor_calc_downward_expander_gain(double detector_val, double threshold,
    double rratio, double kneewidth, bool gate)
{
    double es = 1.0/rratio - 1;
    if (gate)
        es = -1;
    if(kneewidth > 0 && detector_val > (threshold - kneewidth/2.0) &&
            detector_val < threshold + kneewidth/2.0)
    {
        double x[2];
        double y[2];
        x[0] = threshold - kneewidth / 2.0;
        x[1] = threshold + kneewidth / 2.0;
        x[1] = min(0, x[1]);
        y[0] = es;
        y[1] = 0;

        es = lagrpol(&x[0], &y[0], 2, detector_val);
    }
    double yg = es * (threshold - detector_val);
    yg = min(0, yg);
    return pow(10.0, yg/20.0);
}

void dynamics_processor_set_inputgain_db(dynamics_processor *dp, double val)
{
    if (val >= -12 && val <= 20)
        dp->m_inputgain_db = val;
    else
        printf("Val must be between -12 and 20\n");
}

void dynamics_processor_set_threshold(dynamics_processor *dp, double val)
{
    if (val >= -60 && val <= 0)
        dp->m_threshold = val;
    else
        printf("Val must be between -60 and 0\n");
}

void dynamics_processor_set_attack_ms(dynamics_processor *dp, double val)
{
    if (val >= 1 && val <= 300) {
        dp->m_attack_ms = val;
        envelope_detector_setattacktime(&dp->m_left_detector, val);
        envelope_detector_setattacktime(&dp->m_right_detector, val);
    }
    else
        printf("Val must be between 1 and 300\n");
}

void dynamics_processor_set_release_ms(dynamics_processor *dp, double val)
{
    if (val >= 20 && val <= 5000) {
        dp->m_release_ms = val;
        envelope_detector_setreleasetime(&dp->m_left_detector, val);
        envelope_detector_setreleasetime(&dp->m_right_detector, val);
    }
    else
        printf("Val must be between 20 and 5000\n");
}

void dynamics_processor_set_ratio(dynamics_processor *dp, double val)
{
    if (val >= 1 && val <= 20)
        dp->m_ratio = val;
    else
        printf("Val must be between 1 and 20\n");
}

void dynamics_processor_set_outputgain_db(dynamics_processor *dp, double val)
{
    if (val >= 0 && val <= 20)
        dp->m_outputgain_db = val;
    else
        printf("Val must be between 0 and 20\n");
}

void dynamics_processor_set_knee_width(dynamics_processor *dp, double val)
{
    if (val >= 0 && val <= 20)
        dp->m_knee_width = val;
    else
        printf("Val must be between 0 and 20\n");
}

void dynamics_processor_set_lookahead_delay_ms(dynamics_processor *dp,
                                               double val)
{
    if (val >= 0 && val <= 300)
        dp->m_lookahead_delay_ms = val;
    else
        printf("Val must be between 0 and 300\n");
}

void dynamics_processor_set_stereo_link(dynamics_processor *dp,
                                        unsigned int val)
{
    if (val < 2)
        dp->m_stereo_link = val;
    else
        printf("Val must be 0 or 1\n");
}

void dynamics_processor_set_processor_type(dynamics_processor *dp,
                                           unsigned int val)
{
    if (val < 4)
        dp->m_processor_type = val;
    else
        printf("Val must be between 0 and 3\n");
}

void dynamics_processor_set_time_constant(dynamics_processor *dp,
                                          unsigned int val)
{
    if (val < 2) {
        dp->m_time_constant = val;
        if (val == 0) { // digital
            envelope_detector_settcmodeanalog(&dp->m_left_detector, false);
            envelope_detector_settcmodeanalog(&dp->m_right_detector, false);
        }
        else {
            envelope_detector_settcmodeanalog(&dp->m_left_detector, true);
            envelope_detector_settcmodeanalog(&dp->m_right_detector, true);
        }
    }
    else
        printf("Val must be between 0 or 1\n");
}

void dynamics_processor_status(void *self, char *status_string)
{
    dynamics_processor *dp = (dynamics_processor *)self;
    snprintf(status_string, MAX_PS_STRING_SZ,
             "inputgain:%.2f threshold:%.2f attackms:%.2f releasems:%.2f "
             "ratio:%.2f outputgain:%.2f kneewidth:%.2f lookahead:%.2f "
             "sterolink:%s(%d) type:%s(%d) mode:%s(%d)",
             dp->m_inputgain_db, dp->m_threshold, dp->m_attack_ms,
             dp->m_release_ms, dp->m_ratio, dp->m_outputgain_db,
             dp->m_knee_width, dp->m_lookahead_delay_ms,
             dp->m_stereo_link ? "ON" : "OFF", dp->m_stereo_link,
             dynamics_processor_type_to_char[dp->m_processor_type],
             dp->m_processor_type, dp->m_time_constant ? "DIGITAL" : "ANALOG",
             dp->m_time_constant);
}

double dynamics_processor_process(void *self, double input)
{
    double returnval = 0;

    dynamics_processor *dp = (dynamics_processor *)self;
    //double inputgain = pow(10.0, dp->m_inputgain_db / 20.0);
    double outputgain = pow(10.0, dp->m_outputgain_db / 20.0);

    // used in stereo output
    // double xn_l = inputgain * input;
    // double xn_r = 0.;

    double left_detector =
        envelope_detector_detect(&dp->m_left_detector, input);
    double right_detector = left_detector;

    // TODO - something useful with stero

    double link_detector = left_detector;
    double gn = 1.;

    if (dp->m_stereo_link == 0) // on
    {
        link_detector = 0.5 * (pow(10.0, left_detector/20.0) + pow(10.0, right_detector/20.0));
        link_detector = 20.0*log10(link_detector);
    }


    if (dp->m_processor_type == COMP)
        gn = dynamics_processor_calc_compression_gain(
            link_detector, dp->m_threshold, dp->m_ratio, dp->m_knee_width,
            false);
    else if (dp->m_processor_type == LIMIT)
        gn = dynamics_processor_calc_compression_gain(
            link_detector, dp->m_threshold, dp->m_ratio, dp->m_knee_width,
            true);
    else if (dp->m_processor_type == EXPAND)
        gn = dynamics_processor_calc_downward_expander_gain(
            link_detector, dp->m_threshold, dp->m_ratio, dp->m_knee_width,
            false);
    else if (dp->m_processor_type == GATE)
        gn = dynamics_processor_calc_downward_expander_gain(
            link_detector, dp->m_threshold, dp->m_ratio, dp->m_knee_width,
            true);

    double lookahead_out = 0;
    delay_process_audio(&dp->m_left_delay, &input, &lookahead_out);

    returnval = gn * lookahead_out * outputgain;
    //returnval = gn * input * outputgain;
    //returnval = gn * input * inputgain;
    return returnval;
}
