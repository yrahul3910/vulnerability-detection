static void grlib_gptimer_enable(GPTimer *timer)

{

    assert(timer != NULL);





    ptimer_stop(timer->ptimer);



    if (!(timer->config & GPTIMER_ENABLE)) {

        /* Timer disabled */

        trace_grlib_gptimer_disabled(timer->id, timer->config);

        return;

    }



    /* ptimer is triggered when the counter reach 0 but GPTimer is triggered at

       underflow. Set count + 1 to simulate the GPTimer behavior. */



    trace_grlib_gptimer_enable(timer->id, timer->counter + 1);



    ptimer_set_count(timer->ptimer, timer->counter + 1);

    ptimer_run(timer->ptimer, 1);

}
