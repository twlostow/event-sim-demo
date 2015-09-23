#include "sim.h"
#include "vcd.h"

Logic clk_i(1,"clk_i");
Logic counter(8,"counter");

int proc_clk(Context *c)
{
    for(;;)
    {
	c->assign( clk_i, ~clk_i );
	c->wait(10);
    }
    return 0;
}



int proc_counter(Context *c)
{
    for(;;)
    {	
	c->wait_signal(clk_i);

	if(clk_i.pos_edge() )
	{
	    c->assign(counter, counter + Logic::from_int(1));
	}
    }
}

main()
{
    Simulation sim;

    sim.add_signal(&clk_i);
    sim.add_signal(&counter);

    clk_i.initial( Logic::from_int(0) );
    counter.initial( Logic::from_int(0) );

    sim.add_process(proc_clk, "clock_gen", false);
    sim.add_process(proc_counter, "counter", false);

    VCDWriter writer("test_counter.vcd", &sim);
    
    printf("Running simulation...\n");

    sim.run(400); 
    return 0;
}