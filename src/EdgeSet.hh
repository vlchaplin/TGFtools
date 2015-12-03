/*
 *  EdgeSet.hh
 *  LightcurveSimulator
 *
 *  Created by Vandiver L. Chapin on 4/5/10.
 *  Copyright 2010 The University of Alabama in Huntsville. All rights reserved.
 *
 */
 
#ifndef EDGESET_HH
#define EDGESET_HH

#ifndef EDGEDEBUG
 #define EDGEDEBUG 0
#endif

#include <string>
#include <iostream>
#include <iomanip>
#include <cmath>

#include "DynMatrix.h"

using namespace std;

/***********************

Class: EdgeFunction

A class which encapsulates a function which converts a
value of arbitrary precision (type T, specified by client)
to an integer position.  The function parameters, such as min,max,
and number of bins, are stored in the object. The base class
EdgeFunction simply implements linear or log binning (SetLookupMethod =0,1 for linear, 2 for Log).

It is designed to be sub-classed for more complicated combinatoric expressions; only the parameters
and not the functional form can vary at run-time.

The second class, EdgeSet, encapsulates an arbitrary list of edges and does a simple recursive searh
on them.  For large sets of non-uniform binning, such as lightcurves, it is probably best to write
a container of edge functions under which the time range is divided into intervals of homogenous binning.
The container would select the proper function at lower resolution, and the funciton provide high resolution, like a B-tree.
However, tree structures could also be devised to use EdgeSet type lookups instead of functional lookups.

************************/
template<typename T>
class EdgeFunction {

	private:
	int lookup_method;
    T bounds[2];
    T binFactor;
	int edges;


	public:
	EdgeFunction() {
		this->lookup_method = 0;
		this->SetParameters( (T) 0, (T) 0, 1);
	};
	void SetLookupMethod(int i) {
		this->lookup_method = i;
	};
	void getBounds(T * minmax) {
		minmax[0] = bounds[0];
		minmax[1] = bounds[1];
	};
	virtual void SetParameters(T min, T max, int channels) {
		if (min > max) {
			T temp = max;
			max = min;
			min = temp;
		}
		this->bounds[0] = min;
		this->bounds[1] = max;
		this->edges = channels;
		
		//if (min == 0.0) min = 1.0f / (max + 1.0f) ;
        double f = pow( (min / max), -1.0f / double( channels ) );
		this->binFactor = (T) f;
		
	};
	virtual int lookup(T searchval) {
		int bin = 0;
		switch ( lookup_method ) {
            case 1:
                bin = floor( (searchval - bounds[0]) / (bounds[1] - bounds[0] ) );
                break;
				
			case 2:
				T low = bounds[0];
				T high = low*binFactor;
				while ( bin < this->edges && (low <= searchval && high > searchval) ) {
					low = high;
					high *= binFactor;
					bin++;
				}
				break;
				
			default: 
				bin = floor( (searchval - bounds[0]) / (bounds[1] - bounds[0] ) );
				break;
		};
		return bin;
	};
	
	
	friend class EdgeSet;
};

template<typename T>
class EdgeSet : public DynMatrix<T> {
	private:
	int lookup_method;
    T bounds[2];
    double binFactor;

	public:
	
	typedef typename DynMatrix<T>::size_type size_type;
	
	EdgeSet() : DynMatrix<T>(2,1) {
		lookup_method = 0;
	};
	EdgeSet(int channels, T nvalue) : DynMatrix<T>(2,1) {
		reinit( channels, nvalue);
        //MakeLinearBins( (T)10.0, (T)1000.0, channels );
	};
	EdgeSet(T min, T max, int channels) : DynMatrix<T>(2,1) {
        MakeLinearBins( min, max, channels );
	};
	~EdgeSet() {
	
	};
	
	virtual void reinit(int nchan, T blankval) {
		this->DynMatrix<T>::reinit(2, nchan,blankval);
	};
	
	void appendBin(T lowerEdge, T upperEdge) {
		
		if ( this->size() < 1 ) {
			this->DynMatrix<T>::resize(2,1);
			(*this)[0][0] = lowerEdge;
			(*this)[1][0] = upperEdge;
		}
		else {
			vector<T> vec;
			vec.push_back(lowerEdge);
			vec.push_back(upperEdge);
			
			this->addColumn(vec);
		}
		
		this->calcBounds();
	};
	
	void calcBounds() {
		bounds[0] = (*this)[0][0];
		bounds[1] = (*this)[1][this->size()-1];
	};
	
	void getBounds(T * minmax) {
		minmax[0] = bounds[0];
		minmax[1] = bounds[1];
	};
    
    void MakeLinearBins (T min, T max, size_t channels) {
        this->DynMatrix<T>::reinit(2, channels,(T) 0);
		
        T t_channelWidth = (max - min) / T(channels);
        T edge_iter = min;
		
        typename DynMatrix<T>::size_type b;
        b = 0;
		
        if (t_channelWidth == 0) {
			cout << "ERROR EdgeSet<>::MakLinBins() : Channel-width is zero with given parameters\n";
			return;
		}
        
        bounds[0] = min;
        while (b < channels) {
            (*this)[0][b] = edge_iter;
			//cout << edge_iter << ", " << t_channelWidth << ", ";
			edge_iter += t_channelWidth;
			//cout << edge_iter << "\n";
            (*this)[1][b] = edge_iter;
			//cout << (*this)[0][0] << "," << (*this)[1][0] << "\n";
			b++;
        }
        bounds[1] = edge_iter;
        //cout << "---1c---\n"<< *this;
        lookup_method = 1;
    };
    void MakeLogBins (T min, T max, size_type channels) {
		this->DynMatrix<T>::reinit(2, channels,(T) 0);
        if (min == 0.0) min += 1;
        double f = pow( (double)(max / min), 1.0f / double( channels ) );
        T edge_iter = min;
        
		if (f == 0) {
			cout << "ERROR EdgeSet<>::MakLogBins() : Channel-width is zero with given parameters\n";
			return;
		}
		
		typename DynMatrix<T>::size_type b;
        b = 0;

        bounds[0] = min;
        while (b < channels) {
            (*this)[0][b] = edge_iter;
            edge_iter *= f;
            (*this)[1][b] = edge_iter;
            b++;
        }
        bounds[1] = edge_iter;
        
        lookup_method = 2;
        this->binFactor = f;
    };
	
	static inline EdgeSet<T> logbingen(T min, T max, size_type nbins) {
		
		EdgeSet<T> temp;
		temp.MakeLogBins( min, max, nbins );
		return temp;
		
	 }; 
    
	size_type size() {
		size_type j;
		this->DynMatrix<T>::size(NULL,&j);
		return j;
	};
	int findbin (T searchval, T min, T max, int imax) {
		int bin;
		switch ( lookup_method ) {
            case 1:
                bin = floor( (searchval - bounds[0]) / (bounds[1] - bounds[0] ) );
                break;
			default: 
				int guess = floor( (searchval - bounds[0]) / (bounds[1] - bounds[0] ) );
				bin = binsearch(searchval, min, max, guess, imax);
				break;
		};
		
		#if EDGEDEBUG 
		cout << "Bin search: "<<searchval<<", "<< bin << "\n";
		#endif
		return bin;
		
	};
	int findbin (T searchval) {
		int imax = this->size()-1;
        if (imax == -1) return -1;
		
		return findbin(searchval, bounds[0], bounds[1], imax);
	};
	int binsearch (T& searchval, T& minval, T& maxval, int i, int& imax) {
		
		if ( searchval >= maxval ) return imax;
		else if ( searchval < minval ) return -1;
		
		if ( i > imax ) i = binsearch(searchval, minval, maxval, i-1, imax);
		if ( searchval >= (*this)[0][i] && searchval < (*this)[1][i] ) return i;
		
		if ( searchval < (*this)[0][i] ) i = binsearch(searchval, minval, maxval, i-1, imax);
		else if ( searchval > (*this)[1][i] ) i = binsearch(searchval, minval, maxval, i+1, imax);
		
		return i;
	};
	
	virtual EdgeSet<T>& operator=(EdgeSet<T>& that) {
		if (this->size() != that.size()) { this->reinit(that.size(), (T) 0); }
		
		that.getBounds(this->bounds);
		this->lookup_method = that.lookup_method;
		
		this->DynMatrix<T>::operator=(that);

		return *this;
	};
	
	bool operator==(EdgeSet<T>& that) {
		if (this->size() != that.size()) return 0;
	
		if (this->bounds[0] != that.bounds[0]) return 0;
		if (this->bounds[1] != that.bounds[1]) return 0;
		
		if (this->lookup_method != that.lookup_method) return 0;
		
        //Possibly add code to check each edge value
		return 1;
	};
	
	friend ostream& operator<<(ostream &output, EdgeSet& edges) {
		int bins = edges.size();
		output << "EdgeSet Bins: "<<bins<<"\n";		
		for (int k=0;k<bins;k++) {
			//sprintf(line, "
			output << "[" ;
			output << setw(7) << edges[0][k];
			output << " : ";
			output << setw(7) << edges[1][k];
			output << "]\n";
		}
		output << "\n";
		
		return output;
	};
	
};

#endif