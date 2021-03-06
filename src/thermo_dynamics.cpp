#include "thermo_dynamics.h"
#include "memory.h"
#include "print.h"
#include <math.h>



using namespace MAPP_NS;
/*--------------------------------------------
 constructor
 --------------------------------------------*/
ThermoDynamics::ThermoDynamics(const int __precision):
precision(__precision),
l_int(Print::max_length<int>()+1),
l_double(Print::max_length<double>()+__precision),
qs(NULL),
nqs(0)
{
}
/*--------------------------------------------
 destructor
 --------------------------------------------*/
ThermoDynamics::~ThermoDynamics()
{
    delete [] qs;
}
/*--------------------------------------------
 initiated before a run
 --------------------------------------------*/
void ThermoDynamics::init()
{
    int n=nqs*(l_double+1)+l_int+1+nqs;
    MAPP::print_stdout(" ");
    for(int i=0;i<n;i++)
        MAPP::print_stdout(THERMO_LINE);
    MAPP::print_stdout(" \n");
    
    
    MAPP::print_stdout(THERMO_DLMTR);
    print_header("step",l_int+1);
    
    for(int i=0;i<nqs;i++)
    {
        MAPP::print_stdout(THERMO_DLMTR);
        qs[i].print_header(l_double+1);
    }
    MAPP::print_stdout(THERMO_DLMTR"\n");
    
    
    MAPP::print_stdout(THERMO_DLMTR);
    for(int i=0;i<l_int+1;i++)
        MAPP::print_stdout(THERMO_LINE);
    for(int i=0;i<nqs;i++)
    {
        MAPP::print_stdout(THERMO_DLMTR);
        for(int j=0;j<l_double+1;j++)
            MAPP::print_stdout(THERMO_LINE);
    }
    
    MAPP::print_stdout(THERMO_DLMTR"\n");
    
}
/*--------------------------------------------
 print output
 --------------------------------------------*/
void ThermoDynamics::print(int step_no)
{
    MAPP::print_stdout(THERMO_DLMTR" %0*d ",l_int-1,step_no);
    for(int i=0;i<nqs;i++)
        MAPP::print_stdout(THERMO_DLMTR"%+*.*e ",l_double,precision,*qs[i].ptr);
    
    MAPP::print_stdout(THERMO_DLMTR"\n");
}
/*--------------------------------------------
 finish after a run
 --------------------------------------------*/
void ThermoDynamics::fin()
{
    int n=nqs*(l_double+1)+l_int+1+nqs;
    MAPP::print_stdout(" ");
    for(int i=0;i<n;i++)
        MAPP::print_stdout(THERMO_LINE);
    MAPP::print_stdout(" \n");
}
/*--------------------------------------------
 
 --------------------------------------------*/
ThermoQuantity::ThermoQuantity():
name(NULL),
name_lngth(0),
ptr(NULL)
{
}
/*--------------------------------------------
 
 --------------------------------------------*/
ThermoQuantity::~ThermoQuantity()
{
}
/*--------------------------------------------
 
 --------------------------------------------*/
ThermoQuantity::ThermoQuantity(const char* __name,type0& val):
name(__name),
name_lngth(static_cast<int>(strlen(__name))),
ptr(&val)
{
}
/*--------------------------------------------
 
 --------------------------------------------*/
ThermoQuantity::ThermoQuantity(const ThermoQuantity& r):
name(r.name),
name_lngth(r.name_lngth),
ptr(r.ptr)
{
}
/*--------------------------------------------
 
 --------------------------------------------*/
ThermoQuantity::ThermoQuantity(ThermoQuantity&& r):
name(r.name),
name_lngth(r.name_lngth),
ptr(r.ptr)
{
}
/*--------------------------------------------
 
 --------------------------------------------*/
ThermoQuantity& ThermoQuantity::operator =(const ThermoQuantity& r)
{
    this->~ThermoQuantity();
    new (this) ThermoQuantity(r);
    return *this;
}
/*--------------------------------------------
 
 --------------------------------------------*/
ThermoQuantity& ThermoQuantity::operator =(ThermoQuantity&& r)
{
    this->~ThermoQuantity();
    new(this) ThermoQuantity(std::move(r));
    return *this;
}
/*--------------------------------------------
 
 --------------------------------------------*/
void ThermoQuantity::print_header(int lngth)
{
    
    if(name_lngth<=lngth)
        MAPP::print_stdout("%*s%*s",(lngth-lngth/2)+name_lngth/2,name,lngth/2-name_lngth/2,"");
    else
        for(int i=0;i<lngth;i++) MAPP::print_stdout("%c",name[i]);
}

