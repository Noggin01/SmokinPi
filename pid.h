typedef struct 
{
    double windup_guard;
    double proportional_gain;
    double integral_gain;
    double derivative_gain;
    double prev_error;
    double int_error;
    double control;
} pid_type;
 

void Pid_Reset(pid_type* pid);
void Pid_Update(pid_type* pid, double current_error, double dt);
