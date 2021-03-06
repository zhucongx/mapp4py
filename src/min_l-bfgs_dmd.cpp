#include "min_l-bfgs_dmd.h"
#include "memory.h"
#include "dynamic_dmd.h"
#include "thermo_dynamics.h"
#include "ff_dmd.h"
#include "MAPP.h"
using namespace MAPP_NS;
/*--------------------------------------------
 constructor
 --------------------------------------------*/
MinLBFGSDMD::MinLBFGSDMD(int __m,type0 __e_tol,
bool(&__H_dof)[__dim__][__dim__],bool __affine,type0 __max_dx,type0 __max_dalpha,LineSearch* __ls):
MinCGDMD(__e_tol,__H_dof,__affine,__max_dx,__max_dalpha,__ls),
m(__m),
rho(NULL),
alpha(NULL),
s_ptr(NULL),
y_ptr(NULL)
{
}
/*--------------------------------------------
 destructor
 --------------------------------------------*/
MinLBFGSDMD::~MinLBFGSDMD()
{
}
/*--------------------------------------------
 init before a run
 --------------------------------------------*/
void MinLBFGSDMD::init()
{
    chng_box=false;
    Algebra::DoLT<__dim__>::func([this](int i,int j)
    {
        if(H_dof[i][j]) chng_box=true;
    });

    
    try
    {
        MinHelper::CondB<>::init(*this,chng_box,!affine,ALPHA_DOF,C_DOF);
    }
    catch(std::string& err_msg)
    {
        throw err_msg;
    }
}
/*--------------------------------------------
 run
 --------------------------------------------*/
void MinLBFGSDMD::run(int nsteps)
{
    MinHelper::CondLS<LineSearchBrent,LineSearchGoldenSection,LineSearchBackTrack>::run(*this,nsteps,ls,chng_box,!affine,ALPHA_DOF,C_DOF,ntally!=0,xprt!=NULL);
}
/*--------------------------------------------
 fin after a run
 --------------------------------------------*/
void MinLBFGSDMD::fin()
{
    MinHelper::CondB<>::fin(*this,chng_box,!affine,ALPHA_DOF,C_DOF);
}
/*------------------------------------------------------------------------------------------------------------------------------------
 
 ------------------------------------------------------------------------------------------------------------------------------------*/
PyObject* MinLBFGSDMD::__new__(PyTypeObject* type,PyObject* args,PyObject* kwds)
{
    Object* __self=reinterpret_cast<Object*>(type->tp_alloc(type,0));
    PyObject* self=reinterpret_cast<PyObject*>(__self);
    return self;
}
/*--------------------------------------------
 
 --------------------------------------------*/
int MinLBFGSDMD::__init__(PyObject* self,PyObject* args,PyObject* kwds)
{
    FuncAPI<int,type0,symm<bool[__dim__][__dim__]>,bool,type0,type0,OP<LineSearch>> f("__init__",{"m","e_tol","H_dof","affine","max_dx","max_dalpha","ls"});
    f.noptionals=7;
    f.logics<0>()[0]=VLogics("ge",0);
    f.logics<1>()[0]=VLogics("ge",0.0);
    f.logics<4>()[0]=VLogics("gt",0.0);
    f.logics<5>()[0]=VLogics("gt",0.0);
    
    //set the defualts
    f.val<0>()=2;
    f.val<1>()=sqrt(std::numeric_limits<type0>::epsilon());
    for(int i=0;i<__dim__;i++) for(int j=0;j<__dim__;j++)f.val<2>()[i][j]=false;
    f.val<3>()=false;
    f.val<4>()=1.0;
    f.val<5>()=0.1;
    PyObject* empty_tuple=PyTuple_New(0);
    PyObject* empty_dict=PyDict_New();
    PyObject* __ls=LineSearchBackTrack::__new__(&LineSearchBackTrack::TypeObject,empty_tuple,empty_dict);
    LineSearchBackTrack::__init__(__ls,empty_tuple,empty_dict);
    Py_DECREF(empty_dict);
    Py_DECREF(empty_tuple);
    f.val<6>().ob=__ls;
    
    
    if(f(args,kwds)==-1) return -1;
    
    
    
    Object* __self=reinterpret_cast<Object*>(self);
    Py_INCREF(f.val<6>().ob);
    __self->ls=reinterpret_cast<LineSearch::Object*>(f.val<6>().ob);
    __self->min=new MinLBFGSDMD(f.val<0>(),f.val<1>(),f.val<2>(),f.val<3>(),f.val<4>(),f.val<5>(),&(__self->ls->ls));
    __self->xprt=NULL;
    
    return 0;
}
/*--------------------------------------------
 
 --------------------------------------------*/
PyObject* MinLBFGSDMD::__alloc__(PyTypeObject* type,Py_ssize_t)
{
    Object* __self=new Object;
    Py_TYPE(__self)=type;
    Py_REFCNT(__self)=1;
    __self->min=NULL;
    __self->ls=NULL;
    __self->xprt=NULL;
    return reinterpret_cast<PyObject*>(__self);
}
/*--------------------------------------------
 
 --------------------------------------------*/
void MinLBFGSDMD::__dealloc__(PyObject* self)
{
    Object* __self=reinterpret_cast<Object*>(self);
    delete __self->min;
    __self->min=NULL;
    if(__self->ls) Py_DECREF(__self->ls);
    __self->ls=NULL;
    if(__self->xprt) Py_DECREF(__self->xprt);
    __self->xprt=NULL;
    delete __self;
}
/*--------------------------------------------*/
PyTypeObject MinLBFGSDMD::TypeObject ={PyObject_HEAD_INIT(NULL)};
/*--------------------------------------------*/
int MinLBFGSDMD::setup_tp()
{
    TypeObject.tp_name="mapp4py.dmd.min_lbfgs";
    TypeObject.tp_doc=R"---(
    __init__(m=2,e_tol=1.0e-8,H_dof=[[False],[False,False],[False,False,False]],affine=False,max_dx=1.0,max_dalpha=0.1,ls=mapp4py.dmd.ls_bt())
    
    L-BFGS minimization algorithm
    
    Parameters
    ----------
    m : int
       Maximum number of vectors to be stored in memory
    e_tol : double
       Energy tolerance criterion for stopping minimization
    H_dof : symm<bool[dim][dim]>
       Unitcell degrees of freedom during minimization, here dim is the dimension of simulation
    affine : bool
       If set to True atomic displacements would be affine
    max_dx : double
       Maximum displacement of any atom in one step of minimization
    max_dalpha : double
       Maximum change in alpha component of any atom in one step of minimization
    ls : mapp4py.ls
       Line search method
    
    Notes
    -----
    Limited memory Broyden–Fletcher–Goldfarb–Shanno (L-BFGS) algorithm for minimization.
    
    )---";
    
    TypeObject.tp_flags=Py_TPFLAGS_DEFAULT;
    TypeObject.tp_basicsize=sizeof(Object);
    
    TypeObject.tp_new=__new__;
    TypeObject.tp_init=__init__;
    TypeObject.tp_alloc=__alloc__;
    TypeObject.tp_dealloc=__dealloc__;
    setup_tp_methods();
    TypeObject.tp_methods=methods;
    setup_tp_getset();
    TypeObject.tp_getset=getset;
    
    TypeObject.tp_base=&MinCGDMD::TypeObject;
    
    int ichk=PyType_Ready(&TypeObject);
    if(ichk<0) return ichk;
    Py_INCREF(&TypeObject);
    GET_WRAPPER_DOC(TypeObject,__init__)=NULL;
    return ichk;
}
/*--------------------------------------------*/
PyGetSetDef MinLBFGSDMD::getset[]=EmptyPyGetSetDef(2);
/*--------------------------------------------*/
void MinLBFGSDMD::setup_tp_getset()
{
    getset_m(getset[0]);
}
/*--------------------------------------------*/
PyMethodDef MinLBFGSDMD::methods[]=EmptyPyMethodDef(1);
/*--------------------------------------------*/
void MinLBFGSDMD::setup_tp_methods()
{
}
/*--------------------------------------------
 
 --------------------------------------------*/
void MinLBFGSDMD::getset_m(PyGetSetDef& getset)
{
    getset.name=(char*)"m";
    getset.doc=(char*)R"---(
    (int) Maximum No. of vectors
    
    Maximum number of vectors to be stored in memory
    )---";

    getset.get=[](PyObject* self,void*)->PyObject*
    {
        return var<int>::build(reinterpret_cast<Object*>(self)->min->m);
    };
    getset.set=[](PyObject* self,PyObject* op,void*)->int
    {
        VarAPI<int> m("m");
        m.logics[0]=VLogics("ge",0);
        int ichk=m.set(op);
        if(ichk==-1) return -1;
        reinterpret_cast<Object*>(self)->min->m=m.val;
        return 0;
    };
}


