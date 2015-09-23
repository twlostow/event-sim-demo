#include "sim.h"
#include "vcd.h"

Logic clk(1,"clk");
Logic sign(1,"sign");
Logic dividend(32, "dividend");
Logic divider(32, "divider");
Logic quotient(32, "quotient");
Logic remainder(32, "remainder");
Logic ready(1,"ready");
Logic start(1,"start");

Logic divider_copy (64,"divider_copy") ;
Logic dividend_copy (64,"dividend_copy") ;
Logic negative_output(1,"negative_output");
Logic bit(6,"bit");

/*
Code translated from the following example

// Unsigned/Signed division based on Patterson and Hennessy's
algorithm.
// Copyrighted 2002 by studboy-ga / Google Answers.  All rights
reserved.
// Description: Calculates quotient.  The "sign" input determines
whether
//              signs (two's complement) should be taken into
consideration.

module divide(ready,quotient,remainder,dividend,divider,sign,clk);

   input         clk;
   input         sign;
   input [31:0]  dividend, divider;
   output [31:0] quotient, remainder;
   output        ready;

   reg [31:0]    quotient, quotient_temp;
   reg [63:0]    dividend_copy, divider_copy, diff;
   reg           negative_output;
   
   wire [31:0]   remainder = (!negative_output) ? 
                             dividend_copy[31:0] : 
                             ~dividend_copy[31:0] + 1'b1;

   reg [5:0]     bit; 
   wire          ready = !bit;

   initial bit = 0;
   initial negative_output = 0;

   always @( posedge clk ) 

     if( ready ) begin

        bit = 6'd32;
        quotient = 0;
        quotient_temp = 0;
        dividend_copy = (!sign || !dividend[31]) ? 
                        {32'd0,dividend} : 
                        {32'd0,~dividend + 1'b1};
        divider_copy = (!sign || !divider[31]) ? 
                       {1'b0,divider,31'd0} : 
                       {1'b0,~divider + 1'b1,31'd0};

        negative_output = sign &&
                          ((divider[31] && !dividend[31]) 
                        ||(!divider[31] && dividend[31]));
        
     end 
     else if ( bit > 0 ) begin

        diff = dividend_copy - divider_copy;

        quotient_temp = quotient_temp << 1;

        if( !diff[63] ) begin

           dividend_copy = diff;
           quotient_temp[0] = 1'd1;

        end

        quotient = (!negative_output) ? 
                   quotient_temp : 
                   ~quotient_temp + 1'b1;

        divider_copy = divider_copy >> 1;
        bit = bit - 1'b1;

     end
endmodule

*/



int comb1(Context *c)
{
    for(;;)
    {
	std::set<SigBase*> sense_list;

	sense_list.insert(&negative_output);
	sense_list.insert(&dividend_copy);
	sense_list.insert(&bit);

	c->wait_signal( sense_list );

	c->assign ( remainder, 
		(!negative_output.value()) ? 
                             dividend_copy.range(31,0) : 
                             ~dividend_copy.range(31,0) + Logic::from_int(1) );

	c->assign (ready, !bit);
    }
}

int proc1(Context* c)
{

    for(;;)
    {

	c->wait_signal(clk);
	if(clk.pos_edge())
	{
	    if(start.value() && bit.value() == 0)
	    {
    		c->assign(bit, Logic::from_int(32));
	        c->assign(quotient, Logic::from_int(0));

    		c->assign(dividend_copy, (!sign || !dividend.range(31)).value() ? 
            		        concat(Logic(32).set_value(0), dividend) : 
                    		concat(Logic(32).set_value(0), ~dividend + Logic(1).set_value(1) ) );


    		c->assign(divider_copy, (!sign || !divider.range(31)).value() ? 
            		        concat(concat(Logic(1).set_value(0), divider), Logic(31).set_value(0) ) : 
                    		concat(concat(Logic(1).set_value(0), ~divider + Logic(1).set_value(1) ), Logic(31).set_value(0) ));

		c->assign(negative_output, 

		     sign &&
                          ((divider.range(31) && !dividend.range(31)) 
                        ||(!divider.range(31) && dividend.range(31))));
        


	    } else if (bit.value() > 0) {

		Logic diff(64);

		diff = dividend_copy - divider_copy;


		Logic quotient_temp(64);
		
    		if( (!diff.range(63)).value() ) 
		{
 		   c->assign( dividend_copy, diff );
            	  quotient_temp = concat(quotient.range(30,0), Logic(1).set_value(1) );
		} else {

        	  quotient_temp = concat(quotient.range(30,0), Logic(1).set_value(0) );

		}

	        c->assign(quotient, (!negative_output).value() ? 
                   quotient_temp : 
                   ~quotient_temp + Logic(1).set_value(1) );

	        c->assign(divider_copy, divider_copy >> 1);
	        c->assign(bit, bit -  Logic(1).set_value(1) );
	    }
	    
	}
    }
}

int proc_clk(Context *c)
{
    for(;;)
    {
	c->assign( clk, ~clk );
	c->wait(10);
    }
    return 0;
}

int proc_stimulus(Context *c)
{
    int a = 1000, b = 23;

    for(int i = 0; i< 3;i++)
	c->wait_posedge(clk);

    c->assign(dividend, Logic::from_int(a));
    c->assign(divider,Logic::from_int(b));
    c->assign(sign, Logic::from_int(0));
    c->assign(start, Logic::from_int(1));

    c->wait_posedge(clk);

    c->assign(start, Logic::from_int(0));
    c->wait_posedge(clk);


    while( (!ready).value() )
        c->wait_posedge(clk);

    printf("%d / %d = %d (should be %d)\n", a,b,quotient.value(), a/b );
    printf("%d %% %d = %d (should be %d)\n", a,b,remainder.value(), a%b );


    c->finish();
}


main()
{
    Simulation sim;

    sim.add_signal(&clk);
    sim.add_signal(&sign);
    sim.add_signal(&dividend);
    sim.add_signal(&divider);
    sim.add_signal(&quotient);
    sim.add_signal(&remainder);
    sim.add_signal(&ready);
    sim.add_signal(&start);

    sim.add_signal(&divider_copy);
    sim.add_signal(&dividend_copy);
    sim.add_signal(&negative_output);
    sim.add_signal(&bit);

    bit.initial(Logic::from_int(0));
    clk.initial(Logic::from_int(0));
    negative_output.initial(Logic::from_int(0));


    sim.add_process(proc_clk, "clock_gen", false);
    sim.add_process(comb1, "comb1", false);
    sim.add_process(proc1, "proc1", false);
    sim.add_process(proc_stimulus, "proc_stimulus", false);

    VCDWriter writer("test_divide.vcd", &sim);
    
    printf("Running simulation...\n");

    sim.run(2000); 

    return 0;
}

