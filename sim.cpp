#include "sim.h"
#include "vcd.h"

int SigBase::m_staticSigId = 0;


void Simulation::add_process( int (*proc)(Context *), const std::string name, bool continuous )
{
    Context *ctx = new Context;
    ctx->m_state = continuous ? Context::CONTINUOUS : Context::IDLE;
    ctx->m_cofunc = COROUTINE<int, Context*> (proc);
    ctx->m_sim = this;
    ctx->m_name = name;
    m_ctxs.push_back(ctx);
}


void Simulation::run(int64_t units)
{
    if(m_writer)
	m_writer->dump_signals();

    while(m_time < units)
    {
	step();
	if(m_writer)
	    m_writer->dump_signals();
    }

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
        	    TRACE("%-8d: run %s state %d wu %lld\n", m_time, ctx->m_name.c_str(), ctx->m_state );

		    ctx->eval();
		    break;

		case Context::WAITING_TIME:
		    if ( m_time == ctx->m_wait_until )
		    {
            		TRACE("%-8d: resume_wait %s state %d wu %lld\n", m_time, ctx->m_name.c_str(), ctx->m_state, ctx->m_wait_until );

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
			TRACE("%-8d: resume_event %s state %d trigger %s\n", m_time, ctx->m_name.c_str(),  ctx->m_state, trigger->m_name.c_str() );
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


	    TRACE("%-8d: update signal %s [%d drivers] \n", m_time, sig->m_name.c_str(), n_drivers);

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

//    printf("next T %lld\n", min_wait);
    m_time = min_wait;
}

