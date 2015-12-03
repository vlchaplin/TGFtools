/*
 *  TTEventTable.h
 *  LightCurveSim
 *
 *  Created by Vandiver Chaplin on 2/15/10.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef TTEVTABLE
#define TTEVTABLE

#include <cstdio>
#include <vector>
#include <iostream>
#include <algorithm>


#include "DataRecorder.hh"
#include "EdgeSet.hh"
#include "PHAElements.hh"
#include "PHAStructures.hh"

using namespace std;

template<typename t_type, typename m_type>
class PHAEvent {
    
    public:
    t_type time;
    m_type chan;
    
    PHAEvent(){};
    PHAEvent(t_type& t, m_type& m){
        time = t;
        chan = m;
    };
    //For STL sort (only < is needed).
    virtual bool operator< (PHAEvent<t_type,m_type>& that) {
        return (this->time < that.time);
    };
    virtual bool operator< (const PHAEvent<t_type,m_type>& that) const {
        return (this->time < that.time);
    };
    virtual bool operator> (PHAEvent<t_type,m_type>& that) {
        return (this->time > that.time);
    };
	virtual bool operator> (const PHAEvent<t_type,m_type>& that) const {
        return (this->time > that.time);
    };
    virtual bool operator== (PHAEvent<t_type,m_type>& that) {
        return (this->time == that.time);
    };

    void print() {
        char record[40];
        sprintf(record, "%0.6lf, %d \n", time, chan);
        cout << "Event time, magnitude: "<< record;
    };
	
	friend class TTE_IO;
};

template<typename T>
class TTE_misc : public OGIP_PHA_misc<T> {
	
	public:
	T tzero;
	T tmin;
	T tmax;

	TTE_misc() : OGIP_PHA_misc<T>() {
		tzero = 0;
	};
};

template<typename t_type, typename m_type, typename enrg_type>
class TTEventTable {

	public:
	typedef TTE_misc<t_type> fields_class;
    typedef PHAEvent<t_type,m_type> event_type;
	typedef typename vector< event_type >::size_type size_type;
	typedef typename vector< event_type >::iterator evitr;
	typedef typename vector< event_type >::iterator iterator;
	
	friend class TTEWriter;

    private:
    /*
    vector< t_type > events;
	vector< m_type > evmags;
    */
	
	DataRecorder * recorder;
    
    vector< event_type > nodes;
    typename vector< event_type >::iterator n_iterator;
	
	fields_class * ogip_fields;
	EdgeSet<enrg_type> * edges;
	
	unsigned long size;
	
	t_type t_bounds[2];
	m_type m_bounds[2];
	
    
    public:
    TTEventTable() {
		this->size=0;
		this->recorder = NULL;
		ogip_fields = new fields_class;
		edges = NULL;
	};
    ~TTEventTable() {
		if ( ogip_fields != NULL && (! ogip_fields->isShared() ) ) delete ogip_fields;
		if ( edges != NULL) delete edges;
	};
	
	evitr begin() {
		return nodes.begin();
	};
	evitr end() {
		return nodes.end();
	};
	size_type n_events() {
		return nodes.size();
	};
	
	void BeginLogging(const char * caller, int srcLine) {
		if (this->recorder != NULL) delete this->recorder;
		this->recorder = new DataRecorder(caller, srcLine, *this);
		this->recorder->linePr = " TTE : ";
		this->recorder->lineBr = "";
	};
	template<typename Op>
	void LogMutation( string& operation, Op& operand) {
		if (this->recorder != NULL) {
			this->recorder->LogMutation( operation, operand );
		}
	};
	template<typename Op>
	void LogMutation( const char * operation, Op& operand) {
		if (this->recorder != NULL) {
			this->recorder->LogMutation( operation, operand );
		}
	};
	
	fields_class * getMiscFields() {
		return this->ogip_fields;
	};
	void setTzero(t_type tzero) {
		this->ogip_fields->tzero = tzero;
		if (this->recorder != NULL) {
			this->recorder->LogMutation( "field->TZERO = ", tzero );
		}
	};
	
	void AttachMBins(EdgeSet<enrg_type> * set) {
		
		if (edges == NULL) {
			this->edges = new EdgeSet<enrg_type>;
		}
		
		*(this->edges) = *set;
		int nbins = set->size();
		if (this->recorder != NULL) {
			this->recorder->LogMutation( "attached Ebounds: ", nbins );
		}
	};
	EdgeSet<enrg_type> * getEdgeSet() {
		return this->edges;
	};
	void getEdgeSet(EdgeSet<enrg_type>& ech) {
		if ( this->edges != NULL) ech = *(this->edges);
	};
	
    void ShareFields( fields_class * fields ) {
		if ( this->ogip_fields != NULL && this->ogip_fields != fields
			&& (! this->ogip_fields->isShared() ) ) 
			delete this->ogip_fields;
			
		this->ogip_fields = fields;
		this->ogip_fields->setShared();
		if (this->recorder != NULL) {
			this->recorder->LogMutation( "sharing fields ", "" );
		}
	};
	
	void reserve(long int& number) {
		this->nodes.reserve( number );
	};
    void Append(t_type time, m_type mag) {
        this->nodes.push_back( event_type(time,mag) );
		this->size++;
		        
		if (this->size <= 1L) {
			this->t_bounds[0] = time;
			this->t_bounds[1] = time;
			this->m_bounds[0] = mag;
			this->m_bounds[1] = mag;
		} else {
			if ( time < this->t_bounds[0] ) this->t_bounds[0] = time;
			else if ( time > this->t_bounds[1] ) this->t_bounds[1] = time;
			if ( mag < this->m_bounds[0] ) this->m_bounds[0] = mag;
			else if ( mag > this->m_bounds[1] ) this->m_bounds[1] = mag;
		}
	
		ogip_fields->tmin = this->t_bounds[0];
		ogip_fields->tmax = this->t_bounds[1];
    };
	
	void Append(t_type * time, m_type * mag, long num) {
		long i;
		i = 0;
		if (this->size == 0L) {
			this->t_bounds[0] = time[i];
			this->t_bounds[1] = time[i];
			this->m_bounds[0] = mag[i];
			this->m_bounds[1] = mag[i];
		};
		
		for (i=0; i<num;i++) {            
            this->nodes.push_back( event_type(time[i],mag[i]) );
			
			if ( time[i] < this->t_bounds[0] ) this->t_bounds[0] = time[i];
			else if ( time[i] > this->t_bounds[1] ) this->t_bounds[1] = time[i];
			if ( mag[i] < this->m_bounds[0] ) this->m_bounds[0] = mag[i];
			else if ( mag[i] > this->m_bounds[1] ) this->m_bounds[1] = mag[i];
			
		};
		this->size += i;
		ogip_fields->tmin = this->t_bounds[0];
		ogip_fields->tmax = this->t_bounds[1];
    };
	
	void Append(vector< t_type >& times, vector< m_type >& mags) {
	
        typename vector< t_type >::iterator ev1;
        typename vector< m_type >::iterator ev2;
    
		if ( times.size() != mags.size() ) {
			mags.resize( times.size );
		};
		if (this->size == 0) {
			this->t_bounds[0] = times[0];
			this->t_bounds[1] = times[0];
			this->m_bounds[0] = mags[0];
			this->m_bounds[1] = mags[0];
		};
		this->size += times.size();
        //this->nodes.reserve( this->size );
		
		ev1 = times.begin();
        ev2 = mags.begin();
        while (ev1 != times.end() ) {
            this->nodes.push_back( event_type( *ev1, *ev2 ) );
			
			if ( *ev1 < this->t_bounds[0] ) this->t_bounds[0] = *ev1;
			else if ( *ev1 > this->t_bounds[1] ) this->t_bounds[1] = *ev1;
			if ( *ev2 < this->m_bounds[0] ) this->m_bounds[0] = *ev2;
			else if ( *ev2 > this->m_bounds[1] ) this->m_bounds[1] = *ev2;
			
            ++ev1;++ev2;
        }
		ogip_fields->tmin = this->t_bounds[0];
		ogip_fields->tmax = this->t_bounds[1];
	};
	
	long length() {
		return this->size;
	};
    void sortEvents() {
        sort( nodes.begin(), nodes.end() );
		if (this->recorder != NULL) {
			this->recorder->LogMutation( "events sorted ", "" );
		}
    };
    void reverse() {
        nodes.reverse();
		if (this->recorder != NULL) {
			this->recorder->LogMutation( "events reversed ", "" );
		}
    };
	
	enrg_type energy(size_type n, unsigned int minOrMax = 0)
	{
		if ( this->edges == NULL ) return -1;
		
		return (*this->edges)[minOrMax][this->nodes[n].chan];
	};
	
	enrg_type energy(PHAEvent<t_type,m_type>& ev, unsigned int minOrMax = 0)
	{
		if ( this->edges == NULL ) return -1;
		return (*this->edges)[minOrMax][ev.chan];
	};
	
	PHAEvent<t_type,m_type>& operator[](size_type n)
	{
		return this->nodes[n];
	};
	
	
	void CopyEvents(float * times, float * mags, t_type tshift=0.0) {
		long i = 0;
		while ( i < this->size ) {
			times[i] = (float)(this->nodes[i].time - tshift);
			
			if ( this->edges != NULL ) {
				size_t ch = this->nodes[i].chan;
				mags[i] = (float)( (*this->edges)[1][ch] + (*this->edges)[0][ch] ) / 2.0;
			}
			else mags[i] = (float)this->nodes[i].chan;
			
			i++;
        }
	};
	
	void CopyAllEvents(t_type * times, m_type * mags) {
		long i = 0;
		while ( i < this->size ) {
			times[i] = this->events[i];
			mags[i] = this->evmags[i];
			i++;
        }
	};
	void BufferEvents(t_type * timesbf, m_type * magsbf, long start, long * bfsize) {		

		if ( start + *bfsize > (long)this->size ) *bfsize = (this->size - start);

		//cout << "Read size: "<< start << " + " << *bfsize << "\n";
		//cout << "Ev size: " << this->events.size() << "\n";
		//cout << "Mg size: " << this->events.size() << "\n";

		if (*bfsize <= 0) {
			//cout << "End of table: " << start << "\n";
			*bfsize = 0;
			return;
		}

        typename vector< event_type >::size_type n;
        n = (typename vector< event_type >::size_type)(start);
		
		long j = 0;
		
		while ( j < *bfsize ) {
            timesbf[j] = this->nodes[n].time;
            magsbf[j] = this->nodes[n].chan;
			++j;
            ++n;
        }
		
		//cout << "i,j,bfsize: "<< t << "," << j <<"," << *bfsize <<"\n";
	};
	
	void TimeHistAlloc( t_type tstart, t_type tstop, t_type tstep, m_type chmin, m_type chmax, double ** cnts, long * nbins, double ** Xpedges=NULL ) {
		
		bool returnEdges=0;
		if (cnts == NULL || nbins == NULL) return;
		if (Xpedges != NULL) returnEdges=1;
		
		evitr ev = this->begin();
		evitr end = this->end();
		
		long j,jmax;
		
		jmax = floor(( tstop - tstart ) / tstep);
		*nbins = jmax+1;
		
		if (jmax < 0) {
			*cnts = NULL;
			return;
		}		
		else {
			*cnts = new double[jmax+1];
			
			if ( returnEdges ) {
				*Xpedges = new double[jmax+2];
				(*Xpedges)[0] = tstart;
			}
			for (j=0;j<=jmax;j++) {
				(*cnts)[j]=0;
				if ( returnEdges ) (*Xpedges)[j+1] = (*Xpedges)[j] + tstep;
			}
		}
		
		while (ev != end) {

			j = floor( (ev->time - tstart) / tstep );
			
			if ( j < 0  || j > jmax ) {
				ev++;
				continue;
			}
			
			if (ev->chan < chmin || ev->chan > chmax) {
				ev++;
				continue;
			}
			
			(*cnts)[j]++;
			ev++;
			
		}
		
	};
	
	template<typename Xtype, typename Htype>
	void addToHistogram( Xtype * edges, Htype * cnts, long nbins, m_type chmin, m_type chmax, t_type tshifted=0.0 ) {
		
		if (cnts == NULL || edges == NULL) return;
	
		evitr ev = this->begin();
		evitr end = this->end();
		
		Xtype tstart, tstep;
		long j;
		
		tstart = edges[0];
		tstep = (edges[nbins] - edges[0]) / (nbins);
	
		cout << "Tstep = " << tstep << endl;
		
		while (ev != end) {

			j = floor( ( Xtype(ev->time - tshifted) - tstart) / tstep );
			
			if ( j < 0  || j > (nbins-1) ) {
				ev++;
				continue;
			}
			
			if (ev->chan < chmin || ev->chan > chmax) {
				ev++;
				continue;
			}
			
			cnts[j]++;
			ev++;
			
		}
	
	};
	
	template<typename Xtype, typename Htype>
	void buildHistogram( t_type tstart, t_type tstop, Xtype tstep, m_type chmin, m_type chmax, Htype ** cnts, long * nbins, Xtype ** Xpedges=NULL, t_type tshift=0.0, bool add=0 ) {
		
		bool returnEdges=0;
		if (cnts == NULL || nbins == NULL) return;
		if (Xpedges != NULL) returnEdges=1;
		
		evitr ev = this->begin();
		evitr end = this->end();
		
		long j,jmax;
				
		jmax = floor( Xtype( tstop - tstart ) / tstep);
		float residue = Xtype( tstop - tstart ) / tstep  - jmax;
		
		if ( residue <= 0.001 ) {
			*nbins = jmax;
			cout << residue <<  ", jmax"<<endl;
		} else {
			*nbins = jmax+1;
		}
		//if ( jmax == 0 ) *nbins = jmax+1;
		//else *nbins = jmax;
		
		if (jmax < 0) {
			*cnts = NULL;
			return;
		}		
		else {
		
			*cnts = new Htype[jmax+1];
			
			if ( returnEdges ) {
				*Xpedges = new Xtype[jmax+2];
				(*Xpedges)[0] = tstart - tshift;
			}
			for (j=0;j<=jmax;j++) {
				(*cnts)[j]=0;
				if ( returnEdges ) (*Xpedges)[j+1] = (*Xpedges)[j] + tstep;
			}
		}
		
		while (ev != end) {

			j = floor( Xtype(ev->time - tstart) / tstep );
			
			if ( j < 0  || j > jmax ) {
				ev++;
				continue;
			}
			
			if (ev->chan < chmin || ev->chan > chmax) {
				ev++;
				continue;
			}
			
			(*cnts)[j]++;
			ev++;
			
		}
		
	};
	
	void Clear() {
		this->nodes.clear();
		this->size = 0;
		if (this->recorder != NULL) {
			this->recorder->LogMutation( " ** Cleared ** ", "" );
		}
	};
	void DeleteEdgeSet() {
		if (this->edges != NULL) {
			delete this->edges;
		}
	};
	
    void print() {
        n_iterator = nodes.begin();
        while (n_iterator != nodes.end() ) {
            n_iterator->print();
            ++n_iterator;
        }
    };
	friend DataRecorder& operator<<(DataRecorder& output, TTEventTable& obj) {
		int size = obj.size;
		stringstream line;
		line << "#TTE Events: " << size;
		output << line;
		line << "t_Range: "<< obj.t_bounds[0] << " : "<< obj.t_bounds[1];
		output << line;
		line <<  "m_Range: "<< obj.m_bounds[0] << " : "<< obj.m_bounds[1];
		output << line;
		
		if (obj.recorder != NULL && ( &output != obj.recorder) ) {
			output << *obj.recorder;
		}
		
		return output;
	};
	
	friend ostream& operator<<(ostream& output, TTEventTable& obj) {
		int size = obj.size;
		stringstream line;
		line << "#TTE Events: " << size << endl;
		output << line.str();
		line.str("");
		line << "t_Range: "<< setprecision(16) << obj.t_bounds[0] << " : "<< setprecision(16) << obj.t_bounds[1]<< endl;
		output << line.str();
		line.str("");
		line <<  "m_Range: "<< obj.m_bounds[0] << " : "<< obj.m_bounds[1]<< endl;
		output << line.str();
		line.str("");
		if (obj.recorder != NULL) {
			line << *obj.recorder << endl;
			output << line.str();
		}
		
		return output;
	};

/*
    friend ostream& operator<<(ostream &output, TTEventTable& obj) {
		int size = obj.size;
		output << "#TTE Events: "<<size<< endl;
		output << "t_Range: "<< obj.t_bounds[0] << " : "<< obj.t_bounds[1]<< endl;
		output << "m_Range: "<< obj.m_bounds[0] << " : "<< obj.m_bounds[1]<< endl;
		
		if (obj.recorder != NULL) output << *(obj.recorder) << endl;
		
		return output;
	};
*/
};


#endif