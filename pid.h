typedef struct 
{
    float windup_guard;
    float proportional_gain;
    float integral_gain;
    float derivative_gain;
    float prev_error;
    float int_error;
    float control;
} pid_type;
 

void Pid_Reset(pid_type* pid);
void Pid_Update(pid_type* pid, float current_error, float dt);
