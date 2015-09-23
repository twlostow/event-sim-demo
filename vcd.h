#ifndef __VCD_H
#define __VCD_H

#include <cstdio>
#include "sim.h"

class VCDWriter
{
    public:
	VCDWriter(const std::string filename, Simulation *s)
	{
	    m_file = fopen(filename.c_str(),"wb");
	    m_sim = s;

	    fprintf(m_file,"$date\n");
	    fprintf(m_file,"Wed Sep 23 14:38:27 2015\n");
	    fprintf(m_file,"$end\n");
	    fprintf(m_file,"$version\n");
	    fprintf(m_file,"none\n");
	    fprintf(m_file,"$end\n");
	    fprintf(m_file,"$timescale\n");
	    fprintf(m_file,"1s\n");
	    fprintf(m_file,"$end\n");
	    fprintf(m_file,"$scope module main $end\n");
	    
	    BOOST_FOREACH(SigBase *s, m_sim->m_signals)
	    {
		if(Logic *l = dynamic_cast<Logic *>(s) )
		{
		    if(l->m_bits==1)
			fprintf(m_file, "$var reg 1 %04x %s $end\n",  l->m_id, l->m_name.c_str() );
		    else
			fprintf(m_file, "$var reg %d %04x %s [%d:0] $end\n", l->m_bits, l->m_id, l->m_name.c_str(), l->m_bits-1 );
		}
	    }
	    fprintf(m_file, "$upscope $end\n");
	    fprintf(m_file, "$enddefinitions $end\n");
	    m_sim->set_writer(this);

	}

	string to_bin(uint64_t value, int bits)
	{
	    string rv;

	    for(int i = bits-1; i >=0; i-- )
		rv += (value & (1ULL<<i)) ? "1" : "0";
	    return rv;
	}

	void dump_signals()
	{
	    fprintf(m_file,"#%d\n", m_sim->m_time);
	    BOOST_FOREACH(SigBase *s, m_sim->m_signals)
	    {
		if(Logic *l = dynamic_cast<Logic *>(s) )
		{
		    fprintf(m_file, "b%s %04x\n", to_bin(l->value(), l->m_bits).c_str(), l->m_id );
		}
	    }
	}

	void finish()
	{
	}

	Simulation *m_sim;
	FILE *m_file;

};
#endif
