#pragma once

#include <string>
#include <stdexcept>
#include <cmath>
#include <sstream>
#include <fstream>
#include <limits>
#include <iomanip>
#include "Summember.h"
#include "Model.h"


typedef unsigned int uint;

template <typename T>
class Entite;

template <typename T>
class Entite
{
public:
	typedef T value_type;

	Entite(Model<T> & model) : model(model) {}

	//////////////////////////////////////////////
	// Adding methods
	
	// When parser finds some string in equation, this method identifies whether it's variable or constant or parameter
	void PutString(std::string str);
	// When parser finds some number in equation, this method saves it as constant
	void PutNumber(std::string number);

	// Functions saving ramp functions found in equation
	void PutRp(std::string var, std::string theta1, std::string theta2, std::string a = 1, std::string b = 0);
	void PutRm(std::string var, std::string theta1, std::string theta2, std::string a = 1, std::string b = 0);
	void PutRpCoor(std::string var, std::string theta1, std::string theta2, std::string y1 = 0, std::string y2 = 1);
	void PutRmCoor(std::string var, std::string theta1, std::string theta2, std::string y1 = 1, std::string y2 = 0);

	// Functions saving sigmoids found in equation
	void PutSp(std::string points, std::string var, std::string k, std::string theta1, std::string a = 1, std::string b = 0);
	void PutSm(std::string points, std::string var, std::string k, std::string theta1, std::string a = 1, std::string b = 0);
	void PutSpInv(std::string points, std::string var, std::string k, std::string theta1, std::string a, std::string b = 0);
	void PutSmInv(std::string points, std::string var, std::string k, std::string theta1, std::string a, std::string b = 0);

	// Functions saving step functions found in equation
    void PutHp(std::string var, std::string theta, std::string a = 1, std::string b = 0);
    void PutHm(std::string var, std::string theta, std::string a = 1, std::string b = 0);
    
    // Functions saving Hill functions found in equation
    void PutHillp(std::string var, std::string theta, std::string n, std::string a = 1, std::string b = 0);
    void PutHillm(std::string var, std::string theta, std::string n, std::string a = 1, std::string b = 0);

	///////

	std::vector<Summember<T> > & GetSummembers()
	{
		return summembers;
	}

	void AddSummember(Summember<T> const & sm)
	{
		summembers.push_back(sm);
	}

	void clear()
	{
		summembers.clear();
	}

	bool empty() const
	{
		return summembers.empty();
	}

	std::size_t size() const
	{
		return summembers.size();
	}

	Summember<T> const & operator[](std::size_t i) const
	{
		return summembers.at(i);
	}

	void negate() {
	    for(uint i = 0 ; i < size(); i++) {
	        summembers.at(i).negate();
	    }
	}


	/////////////////////////////////////
	// Operators

	/*const*/ Entite<T> operator+(/*const*/ Entite<T> & e2);

	const Entite<T>  operator*(/*const*/ Entite<T> &);

	template <class U>
	friend std::ostream& operator<<(std::ostream& out, const Entite<U>& e);


private:

	void PutConst(std::size_t c);
	void PutParam(std::size_t p);
	void PutVar(std::size_t v);

	T getNumberFromString(std::string s);

    Model<T> & model;

	std::vector<Summember<T> > summembers;
};

template <typename T>
T Entite<T>::getNumberFromString(std::string s) {
    T result;
    try {
        result = (T)std::stod(s);
    } catch(std::invalid_argument& ) {
        for(uint i = 0; i < model.getConstants().size(); i++) {
	        if(s.compare(model.getConstant(i).first) == 0) {
	            result = model.getConstant(i).second;
	            break;
	        }
	    }
    }
    return result;
}


template <typename T>
void Entite<T>::PutString(std::string str)
{
	std::size_t i = 0;
	bool run = 1; // one string, one type

    for(; i < model.getVariables().size(); i++) {
        if(str.compare(model.getVariable(i)) == 0) {
            PutVar(i+1);
            run = 0;
            break;
        }
    }

	if (1 == run) {

	    for(i = 0; i < model.getParamNames().size(); i++) {
	        if(str.compare(model.getParamName(i)) == 0) {
	            PutParam(i+1);
	            run = 0;
	            break;
	        }
	    }
	}

	if (1 == run) {
	    for(i = 0; i < model.getConstants().size(); i++) {
	        if(str.compare(model.getConstant(i).first) == 0) {
	            PutConst(i);
	            run = 0;
	            break;
	        }
	    }
	}

}

template <typename T>
void Entite<T>::PutNumber(std::string number)
{
	double cc = std::stod(number);
	T c = (T)cc;

	Summember<T> new_summember;

	new_summember.AddConstant(c);
	AddSummember(new_summember);
}

template <typename T>
void Entite<T>::PutConst(std::size_t c)
{
    Summember<T> new_summember;
	new_summember.AddConstant(model.getConstant(c).second);
	AddSummember(new_summember);
}

template <typename T>
void Entite<T>::PutVar(std::size_t v)
{
    Summember<T> new_summember;
	new_summember.AddVar(v);
	AddSummember(new_summember);
}

template <typename T>
void Entite<T>::PutParam(std::size_t p)
{
    Summember<T> new_summember;
	new_summember.AddParam(p);
	AddSummember(new_summember);
}



template <typename T>
void Entite<T>::PutRp(std::string var, std::string theta1, std::string theta2, std::string a, std::string b)
{
    T y1 = getNumberFromString(theta1) * getNumberFromString(a) + getNumberFromString(b);
    T y2 = getNumberFromString(theta2) * getNumberFromString(a) + getNumberFromString(b);
    Summember<T> new_summember;
    for(uint i = 0; i < model.getVariables().size(); i++) {
        if(var.compare(model.getVariable(i)) == 0) {
            new_summember.AddRamp(i+1,getNumberFromString(theta1),getNumberFromString(theta2),y1,y2,false);
            AddSummember(new_summember);
            return;
        }
    }
}

template <typename T>
void Entite<T>::PutRm(std::string var, std::string theta1, std::string theta2, std::string a, std::string b)
{
    T y1 = getNumberFromString(theta1) * getNumberFromString(a) + getNumberFromString(b);
    T y2 = getNumberFromString(theta2) * getNumberFromString(a) + getNumberFromString(b);
    Summember<T> new_summember;
    for(uint i = 0; i < model.getVariables().size(); i++) {
        if(var.compare(model.getVariable(i)) == 0) {
            new_summember.AddRamp(i+1,getNumberFromString(theta1),getNumberFromString(theta2),y1,y2,true);
            AddSummember(new_summember);
            return;
        }
    }
}

template <typename T>
void Entite<T>::PutRpCoor(std::string var, std::string theta1, std::string theta2, std::string y1, std::string y2)
{
	T yy1 = getNumberFromString(y1);
	T yy2 = getNumberFromString(y2);
	
	if(yy1 > yy2) {
		T temp = yy1;
		yy1 = yy2;
		yy2 = temp;
	}
	
    Summember<T> new_summember;
    for(uint i = 0; i < model.getVariables().size(); i++) {
        if(var.compare(model.getVariable(i)) == 0) {
            new_summember.AddRamp(i+1,getNumberFromString(theta1),getNumberFromString(theta2),yy1,yy2,false);
            AddSummember(new_summember);
            return;
        }
    }
}

template <typename T>
void Entite<T>::PutRmCoor(std::string var, std::string theta1, std::string theta2, std::string y1, std::string y2)
{
	T yy1 = getNumberFromString(y1);
	T yy2 = getNumberFromString(y2);
	
	if(yy1 < yy2) {
		T temp = yy1;
		yy1 = yy2;
		yy2 = temp;
	}
	
    Summember<T> new_summember;
    for(uint i = 0; i < model.getVariables().size(); i++) {
        if(var.compare(model.getVariable(i)) == 0) {
            new_summember.AddRamp(i+1,getNumberFromString(theta1),getNumberFromString(theta2),yy1,yy2,true);
            AddSummember(new_summember);
            return;
        }
    }
}

template <typename T>
void Entite<T>::PutSp(std::string points, std::string var, std::string k, std::string theta, std::string a, std::string b)
{
    T kk = getNumberFromString(k);
    T th = getNumberFromString(theta);
    T aa = getNumberFromString(a);
    T bb = getNumberFromString(b);
    Summember<T> new_summember;
    for(uint i = 0; i < model.getVariables().size(); i++) {
        if(var.compare(model.getVariable(i)) == 0) {
        
        	if(aa > bb) {
        		T temp = aa;
        		aa = bb;
        		bb = temp;
        	}	

            model.AddSigmoid((size_t)getNumberFromString(points),i+1,kk,th,aa,bb);
			new_summember.AddSigmoid(model.GetSigmoidsSize());
			AddSummember(new_summember);
            return;
        }
    }
}

template <typename T>
void Entite<T>::PutSm(std::string points, std::string var, std::string k, std::string theta, std::string a, std::string b)
{
    T kk = getNumberFromString(k);
    T th = getNumberFromString(theta);
    T aa = getNumberFromString(a);
    T bb = getNumberFromString(b);
    Summember<T> new_summember;
    for(uint i = 0; i < model.getVariables().size(); i++) {
        if(var.compare(model.getVariable(i)) == 0) {
        
        	if(aa < bb) {
        		T temp = aa;
        		aa = bb;
        		bb = temp;
        	}	        	
        
            model.AddSigmoid((size_t)getNumberFromString(points),i+1,kk,th,aa,bb,false);
			new_summember.AddSigmoid(model.GetSigmoidsSize());
            AddSummember(new_summember);
            return;
        }
    }
}

template <typename T>
void Entite<T>::PutSpInv(std::string points, std::string var, std::string k, std::string theta, std::string a, std::string b)
{
    T kk = getNumberFromString(k);
    T th = getNumberFromString(theta);
    T aa = getNumberFromString(a);
    T bb = getNumberFromString(b);
    Summember<T> new_summember;
    for(uint i = 0; i < model.getVariables().size(); i++) {
        if(var.compare(model.getVariable(i)) == 0) {
        
        	if(aa > bb) {
        		T temp = aa;
        		aa = bb;
        		bb = temp;
        	}	        	        	
        
            model.AddSigmoid((size_t)getNumberFromString(points),
                                     i+1,
                                     kk,
                                     th + log(aa/bb)/(2*kk),
                                     1/aa,
                                     1/bb,
                                     false);
			new_summember.AddSigmoid(model.GetSigmoidsSize());
            AddSummember(new_summember);
            return;
        }
    }
}

template <typename T>
void Entite<T>::PutSmInv(std::string points, std::string var, std::string k, std::string theta, std::string a, std::string b)
{
    T kk = getNumberFromString(k);
    T th = getNumberFromString(theta);
    T aa = getNumberFromString(a);
    T bb = getNumberFromString(b);
    Summember<T> new_summember;
    for(uint i = 0; i < model.getVariables().size(); i++) {
        if(var.compare(model.getVariable(i)) == 0) {
        
        	if(aa < bb) {
        		T temp = aa;
        		aa = bb;
        		bb = temp;
        	}	        	        	        
        
            model.AddSigmoid((size_t)getNumberFromString(points),
                                     i+1,
                                     kk,
                                     th + log(aa/bb)/(2*kk),
                                     1/aa,
                                     1/bb
                                     );
			new_summember.AddSigmoid(model.GetSigmoidsSize());
			AddSummember(new_summember);
            return;
        }
    }
}

template <typename T>
void Entite<T>::PutHp(std::string var, std::string theta, std::string a, std::string b)
{
	T th = getNumberFromString(theta);
    T aa = getNumberFromString(a);
    T bb = getNumberFromString(b);
    
    T eps = std::numeric_limits<value_type>::epsilon();
    T th1 = th - ( eps == 0 ? 1 : eps );
    T th2 = th + ( eps == 0 ? 1 : eps );
    
    //saving new thresholds th1 and th2
    model.AddThresholdName(var);
    std::stringstream ss;
    ss.precision(20);
    ss << th1;
    model.AddThresholdValue(ss.str());
    std::stringstream ss2;
    ss2.precision(20);    
    ss2 << th2;
    model.AddThresholdValue(ss2.str());
    

    if(aa > bb) {
    	T temp = aa;
    	aa = bb;
    	bb = temp;
    }
    
    Summember<T> new_summember;
    for(uint i = 0; i < model.getVariables().size(); i++) {
        if(var.compare(model.getVariable(i)) == 0) {
			new_summember.AddRamp(i + 1, th1, th2, aa, bb, false);
            AddSummember(new_summember);
            return;
        }
    }
}

template <typename T>
void Entite<T>::PutHm(std::string var, std::string theta, std::string a, std::string b)
{
	T th = getNumberFromString(theta);
    T aa = getNumberFromString(a);
    T bb = getNumberFromString(b);
   
    T eps = std::numeric_limits<value_type>::epsilon();
    T th1 = th - ( eps == 0 ? 1 : eps );
    T th2 = th + ( eps == 0 ? 1 : eps );
    
    //saving new thresholds th1 and th2
    model.AddThresholdName(var);
    std::stringstream ss;
    ss.precision(20);    
    ss << th1;
    model.AddThresholdValue(ss.str());
    std::stringstream ss2;
    ss2.precision(20);    
    ss2 << th2;
    model.AddThresholdValue(ss2.str());
        
    
    if(aa < bb) {
    	T temp = aa;
    	aa = bb;
    	bb = temp;
    }
    
    Summember<T> new_summember;
    for(uint i = 0; i < model.getVariables().size(); i++) {
        if(var.compare(model.getVariable(i)) == 0) {
			new_summember.AddRamp(i+1, th1, th2, aa, bb, true);
            AddSummember(new_summember);
            return;
        }
    }
}

template <typename T>
void Entite<T>::PutHillp(std::string var, std::string theta, std::string n, std::string a, std::string b) {
	
	T th = getNumberFromString(theta);
    T nn = getNumberFromString(n);
    T aa = getNumberFromString(a);
    T bb = getNumberFromString(b);
    
    if(aa > bb) {
    	T temp = aa;
    	aa = bb;
    	bb = temp;
    }
    
    Summember<T> new_summember;
    for(uint i = 0; i < model.getVariables().size(); i++) {
        if(var.compare(model.getVariable(i)) == 0) {
        	model.AddHill(i + 1, th, nn, aa, bb, true);
        	
        	new_summember.AddHill(model.getHills().size());
            AddSummember(new_summember);
            return;
        }
    }
}

template <typename T>
void Entite<T>::PutHillm(std::string var, std::string theta, std::string n, std::string a, std::string b) {
	
	T th = getNumberFromString(theta);
    T nn = getNumberFromString(n);
    T aa = getNumberFromString(a);
    T bb = getNumberFromString(b);
    
    if(aa < bb) {
    	T temp = aa;
    	aa = bb;
    	bb = temp;
    }
    
    Summember<T> new_summember;
    for(uint i = 0; i < model.getVariables().size(); i++) {
        if(var.compare(model.getVariable(i)) == 0) {
        	model.AddHill(i + 1, th, nn, aa, bb, false);
        	
        	new_summember.AddHill(model.getHills().size());
            AddSummember(new_summember);
            return;
        }
    }
}

template <typename T>
/*const*/ Entite<T>  Entite<T>::operator+(/*const*/ Entite<T> & e2)
{
	Entite<T>  e_final(this->model);
	Entite<T> & e1 = *this;

	/*TODO: special cases*/

	std::vector<Summember<T> > & e2_summembers = e2.GetSummembers();
	std::vector<Summember<T> > & e1_summembers = e1.GetSummembers();

	for (typename std::vector<Summember<T> >::iterator it = e1_summembers.begin(); it != e1_summembers.end(); it++) {
		e_final.AddSummember(*it);
	}

	for (typename std::vector<Summember<T> >::iterator it = e2_summembers.begin(); it != e2_summembers.end(); it++) {
		e_final.AddSummember(*it);
	}

	return e_final;
}


template <typename T>
const Entite<T> Entite<T>::operator*(/*const*/ Entite<T> & e2)
{
	Entite<T> & e1 = *this;
	Entite<T>  e_final(this->model);

	/*TODO: special cases*/


	std::vector<Summember<T> > & e1_summembers = e1.GetSummembers();
	std::vector<Summember<T> > & e2_summembers = e2.GetSummembers();

	for (typename std::vector<Summember<T> >::iterator it1 = e1_summembers.begin(); it1 != e1_summembers.end(); it1++) {
		for (typename std::vector<Summember<T> >::iterator it2 = e2_summembers.begin(); it2 != e2_summembers.end(); it2++) {

			Summember<T> summember;

			summember = (*it1) * (*it2);

			e_final.AddSummember(summember);
		}
	}

	return e_final;
}

template <class U>
std::ostream& operator<<(std::ostream& out, const Entite<U>& e) {
    if(!e.empty())
        out << e[0];
    for(uint i = 1; i < e.size(); i++) {
        out << " + " << e[i];
    }
    return out;
}
