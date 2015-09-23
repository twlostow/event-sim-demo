#include <cstdio>
#include <cassert>
#include <string>
#include <stdint.h>
#include <vector>
#include <set>
#include <map>

#include <boost/foreach.hpp>

#include "coroutine.h"

using namespace std;

class SigBase 
{
public:
    SigBase ( const  std::string name = "?") : m_name(name) {}
 
    virtual SigBase *clone() const = 0;
    virtual void copy_value ( const SigBase *b) =0;

    virtual bool changed() const =0;
    virtual void clear_changed() = 0;

    std::vector<SigBase *> m_drivers;

    std::string m_name;

//    SigBase *m_old_value;
};

class Logic : public SigBase
{
public:

    virtual Logic *clone() const
    {
	Logic *l = new Logic;
	l->m_bits= m_bits;
	l->m_value = m_value;
	l->m_old_value = m_value;
	return l;
    }

    virtual void copy_value ( const SigBase *b) 
    {
	
	m_old_value = m_value;
	m_value = static_cast<const Logic *> (b)->m_value;
    }

    virtual bool changed() const
    {
	bool ch = (m_value ^ m_old_value) & ((1ULL<<m_bits) - 1);
//	printf("old_value %d new_value %d ch %d\n", m_value, m_old_value, !!ch);

	return ch;
    }

    void clear_changed() 
    {
	m_old_value = m_value;
    }

    Logic(int bits=1, string name="?"): SigBase(name), m_bits(bits), m_old_value(0), m_value(0) {}

    Logic set_value( int value )
    {
	m_old_value = m_value;
	m_value = value;
	return *this;
    }

    int64_t value()
    {
	return m_value;
    }

    
    void initial(Logic value)
    {
	m_value = value.m_value;
    }

    Logic range(int rmax, int rmin )
    {
	assert (rmax >= rmin);

	uint64_t tmp = m_value >> rmin;

	tmp &= (1ULL << (rmax-rmin+1)) - 1 ;

	Logic rv(rmax-rmin+1);
	rv.m_value = tmp;
	return rv;
    }

    Logic range(int bit)
    {
	return range(bit, bit);
    }

    Logic range(const Logic& bit)
    {
	return range(bit.m_value);
    }
    
    Logic operator-(const Logic& b)
    {
	Logic rv(m_bits);
	rv.m_value = (m_value - b.m_value) & ((1ULL<<m_bits) - 1);
	return rv;
    }

    Logic operator^(const Logic& b)
    {
	Logic rv(m_bits);
	rv.m_value = (m_value ^ b.m_value) & ((1ULL<<m_bits) - 1);
	return rv;
    }


    Logic operator|(const Logic& b)
    {
	Logic rv(m_bits);
	rv.m_value = (m_value | b.m_value) & ((1ULL<<m_bits) - 1);
	return rv;
    }

    Logic operator&(const Logic& b)
    {
	Logic rv(m_bits);
	rv.m_value = (m_value & b.m_value) & ((1ULL<<m_bits) - 1);
	return rv;
    }


    Logic operator||(const Logic& b)
    {
	Logic rv(1);

	bool op1 = (m_value) & ((1ULL<<m_bits) - 1) ;
	bool op2 = (m_value) & ((1ULL<<m_bits) - 1) ;

	rv.m_value = (op1 || op2) ? 1 : 0;
	return rv;
    }

    Logic operator+(const Logic& b)
    {
	Logic rv(m_bits);
	rv.m_value = (m_value + b.m_value) & ((1ULL<<m_bits) - 1);
	return rv;
    }

    static Logic from_int( int value )
    {
	Logic rv(32);
	rv.m_value = value;
	return rv;
    }

    static Logic from_string( const std::string value )
    {
	Logic rv(32);
	rv.m_value = 0;
	return rv;
    }

    bool operator==(const Logic& b) const
    {
	return (m_value == b.m_value);
    }

    bool operator!=(const Logic& b) const
    {
	return (m_value != b.m_value);

    }


    Logic operator~() const
    {
	Logic rv(m_bits);
	rv.m_value = (~m_value ) & ((1ULL<<m_bits) - 1);
	return rv;	
    }

    bool pos_edge() const
    {
	assert(m_bits == 1);
    	return !m_old_value && m_value;
    }

//private:
    int m_bits;
    uint64_t m_value;
    uint64_t m_old_value;

};

Logic concat ( const Logic&a, const Logic& b )
{

}

class Context;

struct Wait
{
    Wait(Context *ctx, int64_t targetTime)
    {
	m_ctx = ctx;
	m_targetTime = targetTime;
    }
	
    
    int64_t m_targetTime;
    Context *m_ctx;
    
    bool operator<(const Wait&b) const
    {
	return m_targetTime < b.m_targetTime;
    }

    bool operator==(const Wait&b) const
    {
	return m_targetTime == b.m_targetTime;
    }

};

class Simulation
{
public:

    Simulation()
    {
	m_time = 0;
    }

    bool do_contexts(bool signals_changed);

    void add_signal( SigBase *sig )
    {
	m_signals.insert(sig);
    }

    void add_process( int (*proc)(Context *), const std::string name, bool continuous  );

    void step();
    void run(int64_t units);

    void schedule_wait( Context *ctx, uint64_t how_much )
    {
//	Wait wait ( ctx, m_time + how_much );
//	m_pendingWaits.insert ( wait );
    }

    int64_t get_time()
    {
	return m_time;
    }



    std::set<SigBase *> m_signals;
    std::set<SigBase *> m_pendingSignals;
//    std::map<SigBase *, SigBase *> m_assigns;
//    std::set<Wait> m_pendingWaits;
    std::vector<Context *> m_ctxs;

    int64_t m_time;
    int m_delta;
};


class SenseList
{
public:
    SenseList (SigBase *s0 = NULL)
    {
	if(!s0)
	    add(s0);
    }

    void add ( SigBase *s )
    {
	m_signals.insert(s);
    }

    bool matches( SigBase *s )
    {
	if(m_signals.empty() )
	    return true;

	return m_signals.find(s) != m_signals.end();
    }
    
    std::set<SigBase* > m_signals;
};

class Context {

public:

    enum State {
	IDLE = 0,
	WAITING_TIME = 1,
	WAITING_EVENT = 2,
	DONE = 3,
	CONTINUOUS = 4
    };

    Context()
    {
	m_state = IDLE;
    }

    template<class T>
	void assign(T& sig, const T value)
    {
	if (sig != value)
	{
	    printf("%-8d: assign %s [%p] value 0x%x\n", m_sim->m_time, sig.m_name.c_str(), &sig, value.m_value);
	    sig.m_drivers.push_back ( value.clone() );
	    m_sim->m_pendingSignals.insert(&sig);
	}
    }

    bool eval()
    {
//	printf("%-8d: eval %p\n", m_sim->m_time, this);
	if(m_cofunc.Running())
	    m_cofunc.Resume();
	else
    	    m_cofunc.Call(this);

	return m_cofunc.Running();
    }

    void wait( int64_t howmuch )
    {
//	m_sim->schedule_wait(this, howmuch);
	printf("%-8d: schedule_wait until %llu\n", m_sim->m_time, m_sim->m_time + howmuch);

	m_wait_until = m_sim->m_time + howmuch;
	m_state = WAITING_TIME;
	m_cofunc.Yield();
    }

    void wait_signal( SigBase& sig )
    {
	m_wait_signals.insert(&sig);
	m_state = WAITING_EVENT;
	m_cofunc.Yield();
    }

    void wait_signal( const std::set<SigBase*>& list )
    {
	m_wait_signals = list;
	m_state = WAITING_EVENT;
	m_cofunc.Yield();
    }

/*    const SenseList& sensitive_to();

    void sensitize( SenseList& events )
    {
	m_senseList = events;
    }

    void sensitize( SigBase *sig )
    {
	m_senseList.add(sig);
    } */

    State m_state;
    SenseList m_senseList;
    COROUTINE<int, Context*> m_cofunc;
    Simulation *m_sim;
    std::set<SigBase *> m_wait_signals;
    uint64_t m_wait_until;
    string m_name;
};

void Simulation::add_process( int (*proc)(Context *), const std::string name, bool continuous )
{
    Context *ctx = new Context;
    ctx->m_state = continuous ? Context::CONTINUOUS : Context::IDLE;
    ctx->m_cofunc = COROUTINE<int, Context*> (proc);
    ctx->m_sim = this;
    ctx->m_name = name;
    m_ctxs.push_back(ctx);
}


Logic clk_i(1,"clk_i"), rst_i;
Logic d_rs1_i(32,"d_rs1_i");
Logic d_rs2_i(32,"d_rs2_i");
Logic q(32,"q"),r(32),n(32),d(32);
Logic n_sign, d_sign;
Logic state(5);
Logic alu_result(32);
Logic alu_op1(32,"alu_op1"), alu_op2(32,"alu_op2");
Logic is_rem;
Logic r_next;
Logic alu_sub(1,"alu_sub");

void proc1(Context *c)
{
    if( state == Logic::from_int(0) )
    {
    	    c->assign(alu_op1, Logic::from_string("'hx") );
	    c->assign(alu_op2, Logic::from_string("'hx") );
    }
    else if( state == Logic::from_int(1) )
    {
	    c->assign(alu_op1, Logic::from_int(0) );
	    c->assign(alu_op2, d_rs1_i);
    }
    else if( state == Logic::from_int(2) )
    {
	    c->assign(alu_op1, Logic::from_int(0) );
	    c->assign(alu_op2, d_rs2_i);
    }
    else if( state == Logic::from_int(35) )
    {
	    c->assign(alu_op1, Logic::from_int(0) );
	    c->assign(alu_op2, q);
    }
    else if( state == Logic::from_int(36) )
    {
	    c->assign(alu_op1, Logic::from_int(0) );
	    c->assign(alu_op2, r);
    }
    else 
    {
	    c->assign(alu_op1, r_next);
	    c->assign(alu_op2, d );
    }


/*	    break;
	case 2:
	    c->assign(alu_op1, Logic<32>.value(0) );
	    c->assign(alu_op2, d_rs2_i);

	case 35:
	    c->assign(alu_op1, Logic<32>.value(0) );
	    c->assign(alu_op2, q);

        default:
	    c->assign(alu_op1, r_next );
	    c->assign(alu_op2, d );
    }
*/
}

Logic alu_ge(1,"alu_ge");
Logic start_divide(1,"start_divide");
Logic x_stall_i(1);
Logic x_kill_i(1);
Logic d_valid_i(1);
Logic d_is_divide_i(1);
Logic done(1);
Logic x_stall_req_o(1);

int cont1(Context *c)
{
    c->assign(r_next, concat( r.range(30,0), n.range(Logic::from_int(31) - (state - Logic::from_int(3))) ));
    c->assign(alu_result, 
	alu_sub.value() 
	? concat( Logic(1).set_value(0), alu_op1 ) - concat( Logic(1).set_value(0), alu_op2 ) 
	: concat( Logic(1).set_value(0), alu_op1 ) + concat( Logic(1).set_value(0), alu_op2 ) );

    c->assign(alu_ge, ~alu_result.range(32)); //wire alu_ge = ~alu_result [32];
    c->assign(start_divide, ~x_stall_i || ~x_kill_i || d_valid_i || d_is_divide_i);
    c->assign(done, (is_rem.value() ? state == Logic::from_int(37) : state == Logic::from_int(36) ) ? Logic::from_int(1) : Logic::from_int(0) );
    c->assign(x_stall_req_o, start_divide || ~done);

}

int cont2(Context *c)
{   
    switch(state.m_value)
    {
       case 1:
        c->assign(alu_sub, n_sign); break;
       case 2:
	c->assign(alu_sub, d_sign); break;
       case 35:
	c->assign(alu_sub, n_sign ^ d_sign); break;
       case 36:
	c->assign(alu_sub, n_sign); break;
       default:
	c->assign(alu_sub, Logic::from_int(1) ); break;
    }
}

int proc3(Context *c)
{
    c->wait_signal(clk_i);
    
    if(clk_i.pos_edge() )
    {
	if( (rst_i || done).value() )
	    c->assign(state, Logic::from_int(0) );
	else if ( (Logic(1).set_value(state != Logic::from_int(0)) || start_divide).value() )
	    c->assign(state, state + Logic::from_int(1) );
    }
}

#if 0


   
   always@(posedge clk_i)
      case ( state ) // synthesis full_case parallel_case
        0:
          if(start_divide)
          begin
	 q <= 0;
	 r <= 0;

	 is_rem <= (d_fun_i == `FUNC_REM || d_fun_i ==`FUNC_REMU);
	 
	 n_sign <= d_rs1_i[31];
	 d_sign <= d_rs2_i[31];
          end

        1: 
	n <= alu_result[31:0];
      
        2: 
	d <= alu_result[31:0];

        35:
          x_rd_o <= alu_result; // quotient

        36:
          x_rd_o <= alu_result; // remainder

        default: // 3..34: 32 divider iterations
          begin
	 
	 q <= { q[30:0], alu_ge };
	 r <= alu_ge ? alu_result : r_next;
	 
	 
          end
      endcase // case ( state )
   
Sig<int> clk(0);


voic 

main()
{

}

#endif

int proc_clk(Context *c)
{
    for(;;)
    {
	c->assign( clk_i, ~clk_i );
	c->wait(10);
    }
    return 0;
}

void Simulation::run(int64_t units)

{

    while(m_time < units)
	step();
}

/*
1. While there are postponed processes:
(a) Pick one or more postponed processes to execute (become active).
(b) Provisionally execute assignments (new values become visible at step 3)
(c) A process executes until it hits its sensitivity list or a wait statement, at which point it
suspends.
(d) Processes that become suspended, stay suspended until there are no more postponed
or active processes.
2. Each process checks its sensitivity list or wait condition to see if it should resume
3. Update signals with their provisional values
4. If no postponed processes, then increment simulation time to next event.

*/

bool Simulation::do_contexts(bool signals_changed)
{
    std::set<SigBase *> processed_events;

	BOOST_FOREACH(Context *ctx, m_ctxs)
	{
	    if(ctx->m_state == Context::DONE)
		continue;
	
    	    switch(ctx->m_state)
	    {
		case Context::IDLE:
        	    printf("%-8d: run %s state %d wu %lld\n", m_time, ctx->m_name.c_str(), ctx->m_state );

		    ctx->eval();
		    break;

		case Context::WAITING_TIME:
		    if ( m_time == ctx->m_wait_until )
		    {
            		printf("%-8d: resume_wait %s state %d wu %lld\n", m_time, ctx->m_name.c_str(), ctx->m_state, ctx->m_wait_until );

			ctx->m_state == Context::IDLE;
			ctx->eval();
		    }
		    break;
	    
		case Context::WAITING_EVENT:
		{
		    SigBase *trigger = NULL;

		    BOOST_FOREACH( SigBase *sig, ctx->m_wait_signals )
		    {
			if(sig->changed())
			{
			    trigger = sig;
			    processed_events.insert(trigger);
			    break;
			}



		    }

        	    if(trigger)
		    {
			printf("%-8d: resume_event %s state %d trigger %s\n", m_time, ctx->m_name.c_str(),  ctx->m_state, trigger->m_name.c_str() );
			ctx->m_state == Context::IDLE;
			ctx->eval();
		    }
		}
		break;

		default:

		    break;
	    }
//    	    printf("Do context %p state %d wu %lld\n", ctx, ctx->m_state, ctx->m_wait_until);
	}

	BOOST_FOREACH(SigBase *sig, processed_events)
	    sig->clear_changed();

}

void Simulation::step()
{
    m_delta = 0;


    do {
        bool signals_changed = false;

	m_delta++;

	do_contexts(true);

	BOOST_FOREACH(SigBase *sig, m_pendingSignals)
	{

	    if(sig->m_drivers.empty() )
	    {
	        m_pendingSignals.erase(sig);
		continue;
	    }

	    int n_drivers = 0;


	    BOOST_FOREACH(SigBase *drv, sig->m_drivers)
	    {
		n_drivers++;
//		printf(" - drive \n");
//		printf("- drive %s\n", drv->m_value);
		sig->copy_value( drv ); // fixme: support multiple drivers
		delete drv;
	    }


	    printf("%-8d: update signal %s [%d drivers] \n", m_time, sig->m_name.c_str(), n_drivers);

	    if(!sig->changed())
	        m_pendingSignals.erase(sig);
	    else
		signals_changed = true;
	    sig->m_drivers.clear();
	}
	
	if(!signals_changed)
	    break;



    } while(1);

    
    int64_t min_wait = 1000000000;

    BOOST_FOREACH(Context *ctx, m_ctxs)
    {
	if(ctx->m_state == Context::WAITING_TIME)
	{
	    if( ctx->m_wait_until < min_wait )
		min_wait = ctx->m_wait_until;
	}
    }

    printf("next T %lld\n", min_wait);
    m_time = min_wait;
}

Logic counter(8,"counter");

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
    sim.add_signal(&rst_i);
    sim.add_signal(&counter);

    clk_i.initial( Logic::from_int(0) );
    rst_i.initial( Logic::from_int(0) );
    counter.initial( Logic::from_int(0) );

//    sim.add_process(cont1, true);
//    sim.add_process(proc1, false);
    sim.add_process(proc_clk, "clock_gen", false);
    sim.add_process(proc_counter, "counter", false);

    

    sim.run(100);    
}