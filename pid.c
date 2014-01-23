#include "pid.h"

void Pid_Reset(pid_type* pid) 
{
    // set prev and integrated error to zero
    pid->prev_error = 0;
    pid->int_error = 0;
}
 
void Pid_Update(pid_type* pid, double current_error, double dt) 
{
    double diff;
    double p_term;
    double i_term;
    double d_term;

    // integration with windup guarding
    pid->int_error += (curr_error * dt);
    if (pid->int_error < -(pid->windup_guard))
        pid->int_error = -(pid->windup_guard);
    else if (pid->int_error > pid->windup_guard)
        pid->int_error = pid->windup_guard;

    // differentiation
    if (dt != 0)
        diff = ((curr_error - pid->prev_error) / dt);
    else
        dt = 0;

    // scaling
    p_term = (pid->proportional_gain * curr_error);
    i_term = (pid->integral_gain     * pid->int_error);
    d_term = (pid->derivative_gain   * diff);

    // summation of terms
    pid->control = p_term + i_term + d_term;

    // save current error as previous error for next iteration
    pid->prev_error = current_error;
}