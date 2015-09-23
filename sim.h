#ifndef __SIM_H
#define __SIM_H


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

class VCDWriter;


#ifdef DEBUG
#define TRACE(...) printf(__VA_ARGS__)
#else
#define TRACE(...)
#endif

class SigBase 
{
public:
    SigBase ( const  std::string name = "?") : m_name(name) {
	m_id = m_staticSigId++;
    }
 
    static int m_staticSigId;

    virtual SigBase *clone() const = 0;
    virtual void copy_value ( const SigBase *b) =0;

    virtual bool changed() const =0;
    virtual void clear_changed() = 0;

    std::vector<SigBase *> m_drivers;

    std::string m_name;
    int m_id;
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
//	TRACE("old_value %d new_value %d ch %d\n", m_value, m_old_value, !!ch);

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

    uint64_t value() const
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
    


    Logic operator>>(int  b)
    {
	Logic rv(m_bits);
	rv.m_value = (m_value >> b);
	return rv;
    }

    Logic operator<<(int  b)
    {
	Logic rv(m_bits);
	rv.m_value = (m_value << b);
	return rv;
    }


    uint64_t mask(int n_bits)
    {
	if(n_bits==64)
	    return 0xffffffffffffffffULL;
	else
	    return ((1ULL<<m_bits) - 1);
    }

    Logic operator-(const Logic& b)
    {
	Logic rv(m_bits);
	rv.m_value = (m_value - b.m_value) & mask(std::max(m_bits, b.m_bits));
	return rv;
    }

    Logic operator^(const Logic& b)
    {
	Logic rv(m_bits);
	rv.m_value = (m_value ^ b.m_value) & mask(m_bits);
	return rv;
    }


    Logic operator|(const Logic& b)
    {
	Logic rv(m_bits);
	rv.m_value = (m_value | b.m_value) & mask(m_bits);
	return rv;
    }

    Logic operator&(const Logic& b)
    {
	Logic rv(m_bits);
	rv.m_value = (m_value & b.m_value) & mask(m_bits);
	return rv;
    }


    Logic operator||(const Logic& b)
    {
	Logic rv(1);

	bool op1 = (m_value) & mask(m_bits) ;
	bool op2 = (b.m_value) & mask(b.m_bits);

	rv.m_value = (op1 || op2) ? 1 : 0;
	return rv;
    }

    Logic operator&&(const Logic& b)
    {
	Logic rv(1);

	bool op1 = (m_value) & mask(m_bits) ;
	bool op2 = (b.m_value) & mask(b.m_bits);

	rv.m_value = (op1 && op2) ? 1 : 0;
	return rv;
    }

    Logic operator+(const Logic& b)
    {
	Logic rv(m_bits);
	rv.m_value = (m_value + b.m_value) & mask(std::max(m_bits, b.m_bits));
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

    Logic operator!() const
    {
	Logic rv(m_bits);
	rv.m_value = ( m_value == 0 ) ? 1 : 0;
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

static Logic concat ( const Logic&a, const Logic& b )
{
    Logic rv (a.m_bits + b.m_bits);

    rv.m_value = a.m_value << b.m_bits;
    rv.m_value |= b.m_value;

//    printf("Concat %d %d %lx %lx -> %d %lx\n", a.m_bits, b.m_bits, a.m_value, b.m_value, rv.m_bits, rv.m_value);

    return rv;
}

class Context;


class Simulation
{
public:

    Simulation()
    {
	m_time = 0;
	m_writer = NULL;
    }

    bool do_contexts(bool signals_changed);

    void add_signal( SigBase *sig )
    {
	m_signals.insert(sig);
    }

    void add_process( int (*proc)(Context *), const std::string name, bool continuous  );

    void step();
    void run(int64_t units);

    int64_t get_time()
    {
	return m_time;
    }

    void set_writer(VCDWriter *writer)
    {
	m_writer = writer;
    }


    VCDWriter *m_writer;

    std::set<SigBase *> m_signals;
    std::set<SigBase *> m_pendingSignals;
    std::vector<Context *> m_ctxs;

    int64_t m_time;
    int m_delta;
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
	    TRACE("%-8lld: assign %s [%p] value 0x%lx\n", m_sim->m_time, sig.m_name.c_str(), &sig, value.m_value);
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
	TRACE("%-8d: schedule_wait until %lu\n", m_sim->m_time, m_sim->m_time + howmuch);

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

    void wait_posedge ( Logic& sig )
    {
	do
	{
	    wait_signal(sig);
	} while (!sig.pos_edge());
    }

    void finish()
    {
	m_state = DONE;
    }

    State m_state;
    COROUTINE<int, Context*> m_cofunc;
    Simulation *m_sim;
    std::set<SigBase *> m_wait_signals;
    uint64_t m_wait_until;
    string m_name;
};

#endif
