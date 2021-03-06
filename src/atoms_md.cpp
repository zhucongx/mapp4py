#include "atoms_md.h"
#include "xmath.h"
/*--------------------------------------------
 
 --------------------------------------------*/
AtomsMD::AtomsMD(MPI_Comm& world):
Atoms(world),
S_pe{DESIG2(__dim__,__dim__,NAN)},
pe(NAN)
{
    elem=new Vec<elem_type>(this,1,"elem");
    x_d=new Vec<type0>(this,__dim__,"x_d");
    x_d->empty(0.0);
}
/*--------------------------------------------
 copy constructor
 --------------------------------------------*/
AtomsMD::AtomsMD(const AtomsMD& other):
Atoms(other),
S_pe{DESIG2(__dim__,__dim__,NAN)},
pe(other.pe)
{
    memcpy(&S_pe[0][0],&other.S_pe[0][0],__dim__*__dim__*sizeof(type0));
    elem=new Vec<elem_type>(this,*other.elem);
    x_d=new Vec<type0>(this,*other.x_d);
}
/*--------------------------------------------
 
 --------------------------------------------*/
AtomsMD::~AtomsMD()
{
    delete x_d;
    delete elem;
}
/*--------------------------------------------

 --------------------------------------------*/
AtomsMD& AtomsMD::operator=(const AtomsMD& r)
{
    this->~AtomsMD();
    new (this) AtomsMD(r);
    return *this;
}
/*--------------------------------------------

 --------------------------------------------*/
AtomsMD& AtomsMD::operator+(const AtomsMD& r)
{
    import_vecs(r);
    natms_lcl+=r.natms_lcl;
    natms+=r.natms;
    for(int ivec=0;ivec<nvecs;ivec++)
        vecs[ivec]->resize(natms_lcl);
    
    reset_domain();
    return *this;
}
/*--------------------------------------------

 --------------------------------------------*/
AtomsMD& AtomsMD::operator+=(const AtomsMD& r)
{
    this->operator+(r);
    return *this;
}
/*--------------------------------------------
 
 --------------------------------------------*/
void AtomsMD::import_vecs(const AtomsMD& other)
{
    Atoms::import_vecs(other);
    
    elem->append(*other.elem,0);
    x_d->append(*other.x_d,0.0);
    
    int __nelems=static_cast<int>(other.elements.__nelems);
    elem_type* elem_map=__nelems==0? NULL:new elem_type[__nelems];
    for(int i=0;i<__nelems;i++)
        elem_map[i]=elements.add_type(other.elements.masses[i],other.elements.names[i].c_str());
    
    elem_type* elem_ptr=elem->begin()+natms_lcl;
    for(int iatm=0;iatm<other.natms_lcl;iatm++)
        elem_ptr[iatm]=elem_map[elem_ptr[iatm]];
    
    delete [] elem_map;
}
/*--------------------------------------------
 
 --------------------------------------------*/
AtomsMD& AtomsMD::operator=(const Atoms& r)
{
    elements=r.elements;
    comm=r.comm;
    natms_lcl=r.natms_lcl;
    natms_ph=r.natms_ph;
    natms=r.natms;
    step=r.step;
    
    max_cut=r.max_cut;
    kB=r.kB;
    hP=r.hP;
    
    vol=r.vol;
    memcpy(depth_inv,r.depth_inv,__dim__*sizeof(type0));
    memcpy(__h,r.__h,__nvoigt__*sizeof(type0));
    memcpy(__b,r.__b,__nvoigt__*sizeof(type0));
    memcpy(&H[0][0],&r.H[0][0],__dim__*__dim__*sizeof(type0));
    memcpy(&B[0][0],&r.B[0][0],__dim__*__dim__*sizeof(type0));
    
    for(int i=0;i<nvecs;i++)
        if(!vecs[i]->is_empty())
            vecs[i]->resize(natms_lcl);
    memcpy(x->begin(),r.x->begin(),natms_lcl*__dim__*sizeof(type0));
    memcpy(id->begin(),r.id->begin(),natms_lcl*sizeof(id_type));
    return* this;
}
/*--------------------------------------------
 x2s
 --------------------------------------------*/
void AtomsMD::x_d2s_d_dump()
{
    Algebra::X2S_NOCORR<__dim__>(__b,natms,x_d->begin_dump());
}
/*--------------------------------------------
 
 --------------------------------------------*/
#include "random.h"
#include "elements.h"
#include "xmath.h"
void AtomsMD::create_T(type0 T,int seed)
{
    if(x_d->is_empty()) x_d->fill();
    Random rand(seed+comm_rank);
    type0* __x_d=x_d->begin();
    type0* m=elements.masses;
    elem_type* __elem=elem->begin();
    type0 fac;
    if(!x_dof->is_empty())
    {
        bool* __dof=x_dof->begin();
        for(int i=0;i<natms_lcl;i++)
        {
            fac=sqrt(kB*T/m[*__elem]);
            Algebra::Do<__dim__>::func([&fac,&rand,&__x_d,__dof](const int i){if(__dof[i]) __x_d[i]=rand.gaussian()*fac;});
            __x_d+=__dim__;
            __dof+=__dim__;
            ++__elem;
        }
    }
    else
    {
        for(int i=0;i<natms_lcl;i++)
        {
            fac=sqrt(kB*T/m[*__elem]);
            Algebra::Do<__dim__>::func([&fac,&rand,&__x_d](const int i){__x_d[i]=rand.gaussian()*fac;});
            __x_d+=__dim__;
            ++__elem;
        }
    }
}
/*--------------------------------------------
 
 --------------------------------------------*/
int AtomsMD::count_elem(elem_type ielem)
{
    elem_type* __elem=elem->begin();
    int n_lcl=0;
    for(int i=0;i<natms_lcl;i++)
        if(__elem[i]==ielem) n_lcl++;
    int n=0;
    MPI_Allreduce(&n_lcl,&n,1,Vec<int>::MPI_T,MPI_SUM,world);
    return n;
}
/*--------------------------------------------
 
 --------------------------------------------*/
void AtomsMD::DO(PyObject* op)
{   
    class Func_x
    {
    public:
        void operator()(type0*,type0*)
        {};
    };
    Func_x func_x;
    VecPy<type0,Func_x> x_vec_py(x,func_x);
    
    class Func_x_d
    {
    public:
        void operator()(type0*,type0*)
        {};
    };
    Func_x_d func_x_d;
    VecPy<type0,Func_x_d> x_d_vec_py(x_d,func_x_d);
    
    
    class Func_id
    {
    public:
        void operator()(id_type* old_val,id_type* new_val)
        {
            if(*old_val!=*new_val)
                throw std::string("id of atoms cannot be changed");
        };
    };
    Func_id func_id;
    VecPy<id_type,Func_id> id_vec_py(id,func_id);
    
    
    
    class Func_elem
    {
    public:
        elem_type nelem;
        Func_elem(elem_type __nelem):nelem(__nelem){}
        void operator()(elem_type* old_val,elem_type* new_val)
        {
            if(*new_val>=nelem)
                throw std::string("elem of atoms should be less than ")+Print::to_string(static_cast<int>(nelem));
        };
    };
    Func_elem func_elem(elements.__nelems);
    VecPy<elem_type,Func_elem> elem_vec_py(elem,func_elem);
    
    
    
    class Func_dof
    {
    public:
        void operator()(bool*,bool*)
        {
        };
    };
    Func_dof func_dof;
    VecPy<bool,Func_dof> dof_vec_py(x_dof,func_dof);
    try
    {
        VecPyFunc::Do(this,op,id_vec_py,x_vec_py,x_d_vec_py,elem_vec_py,dof_vec_py);
    }
    catch(std::string& err_msg)
    {
        throw err_msg;
    }
    
    if(x_vec_py.inc) this->reset_domain();
    
}
/*------------------------------------------------------------------------------------------------------------------------------------
 
 ------------------------------------------------------------------------------------------------------------------------------------*/
#include "ff_styles.h"
#include "import_styles.h"
#include "analysis.h"
PyObject* AtomsMD::__new__(PyTypeObject* type,PyObject* args,PyObject* kwds)
{
    Object* __self=reinterpret_cast<Object*>(type->tp_alloc(type,0));
    PyObject* self=reinterpret_cast<PyObject*>(__self);
    return self;
}
/*--------------------------------------------
 
 --------------------------------------------*/
int AtomsMD::__init__(PyObject* self,PyObject* args,PyObject* kwds)
{
    FuncAPI<> func("__init__");
    if(func(args,kwds)==-1) return -1;
    Object* __self=reinterpret_cast<Object*>(self);
    __self->atoms=NULL;
    __self->ff=NULL;
    return 0;
}
/*--------------------------------------------
 
 --------------------------------------------*/
PyObject* AtomsMD::__alloc__(PyTypeObject* type,Py_ssize_t)
{
    Object* __self=new Object;
    Py_TYPE(__self)=type;
    Py_REFCNT(__self)=1;
    __self->atoms=NULL;
    __self->ff=NULL;
    return reinterpret_cast<PyObject*>(__self);
}
/*--------------------------------------------
 
 --------------------------------------------*/
void AtomsMD::__dealloc__(PyObject* self)
{
    Object* __self=reinterpret_cast<Object*>(self);
    delete __self->ff;
    delete __self->atoms;
    delete __self;
}
/*--------------------------------------------

--------------------------------------------*/
PyObject* AtomsMD::__add__(PyObject* l,PyObject* r)
{
    PyObject* __l=__new__(&TypeObject,NULL,NULL);
    reinterpret_cast<Object*>(__l)->atoms=new AtomsMD(*(reinterpret_cast<Object*>(l)->atoms));
    PyObject* ans=__iadd__(__l,r);
    Py_DECREF(__l);
    return ans;
}
/*--------------------------------------------

--------------------------------------------*/
PyObject* AtomsMD::__iadd__(PyObject* l,PyObject* r)
{
    VarAPI<OP<AtomsMD>> var("rhs");
    if(!var.set_nothrow(r))
    {
        PyErr_Format(PyExc_TypeError,"unsupported operand type(s) for %s: \'%s\' and \'%s\'","+",l->ob_type->tp_name,r->ob_type->tp_name);
        return NULL;
    }
    Object* __l=reinterpret_cast<Object*>(l);
    AtomsMD* atoms_l=__l->atoms;
    Object* __r=reinterpret_cast<Object*>(r);
    AtomsMD* atoms_r=__r->atoms;
    
    if(atoms_l->natms_ph!=0 || atoms_r->natms_ph!=0)
    {
        PyErr_SetString(PyExc_TypeError,"atom objects have phantom atoms");
        return NULL;
    }
    int is_same;
    MPI_Comm_compare(atoms_l->world,atoms_r->world,&is_same);
    if(is_same!=MPI_IDENT)
    {
        PyErr_SetString(PyExc_TypeError,"atom objects do not belong to same world");
        return NULL;
    }
        
    *(__l->atoms)+=*(__r->atoms);
    Py_INCREF(l);
    return l;
}
/*--------------------------------------------

--------------------------------------------*/
PyObject* AtomsMD::__mul__(PyObject* l,PyObject* r)
{
    PyObject* __l=__new__(&TypeObject,NULL,NULL);
    reinterpret_cast<Object*>(__l)->atoms=new AtomsMD(*(reinterpret_cast<Object*>(l)->atoms));
    PyObject* ans=__imul__(__l,r);
    Py_DECREF(__l);
    return ans;
}
/*--------------------------------------------

--------------------------------------------*/
PyObject* AtomsMD::__imul__(PyObject* l,PyObject* r)
{
    VarAPI<int[__dim__]> var("rhs");
    var.logics[0]=VLogics("ge",1);
    if(var.set(r)!=0)
        return NULL;
    
    Object* __l=reinterpret_cast<Object*>(l);
    __l->atoms->mul(var.val);
    Py_INCREF(l);
    return l;
}
/*--------------------------------------------*/
PyTypeObject AtomsMD::TypeObject ={PyObject_HEAD_INIT(NULL)};
PyNumberMethods AtomsMD::NumberMethods={};
/*--------------------------------------------*/
int AtomsMD::setup_tp()
{
    TypeObject.tp_name="mapp4py.md.atoms";
    TypeObject.tp_doc="container class";
    
    TypeObject.tp_flags=Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
    TypeObject.tp_basicsize=sizeof(Object);
    
    TypeObject.tp_new=__new__;
    TypeObject.tp_init=__init__;
    TypeObject.tp_alloc=__alloc__;
    TypeObject.tp_dealloc=__dealloc__;
    
    NumberMethods.nb_add=__add__;
    NumberMethods.nb_inplace_add=__iadd__;
    NumberMethods.nb_multiply=__mul__;
    NumberMethods.nb_inplace_multiply=__imul__;
    TypeObject.tp_as_number=&NumberMethods;
    
    setup_tp_getset();
    TypeObject.tp_getset=getset;
    setup_tp_methods();
    TypeObject.tp_methods=methods;
        
    int ichk=PyType_Ready(&TypeObject);
    if(ichk<0) return ichk;
    Py_INCREF(&TypeObject);
    return ichk;
}
/*--------------------------------------------*/
PyGetSetDef AtomsMD::getset[]=EmptyPyGetSetDef(16);
/*--------------------------------------------*/
void AtomsMD::setup_tp_getset()
{
    getset_natms(getset[0]);
    getset_step(getset[1]);
    getset_hP(getset[2]);
    getset_kB(getset[3]);
    getset_H(getset[4]);
    getset_B(getset[5]);
    getset_vol(getset[6]);
    getset_elems(getset[7]);
    getset_skin(getset[8]);
    getset_comm_rank(getset[9]);
    getset_comm_size(getset[10]);
    getset_comm_coords(getset[11]);
    getset_comm_dims(getset[12]);
    getset_pe(getset[13]);
    getset_S_pe(getset[14]);
}
/*--------------------------------------------
 
 --------------------------------------------*/
void AtomsMD::getset_S_pe(PyGetSetDef& getset)
{
    getset.name=(char*)"S_pe";
    getset.doc=(char*)R"---(
    (double) potential energy stress
    
    Potential energy part of virial stress
    )---";
    getset.get=[](PyObject* self,void*)->PyObject*
    {
        return var<type0[__dim__][__dim__]>::build(reinterpret_cast<Object*>(self)->atoms->S_pe);
    };
    getset.set=[](PyObject* self,PyObject* val,void*)->int
    {
        PyErr_SetString(PyExc_TypeError,"readonly attribute");
        return -1;
    };
}
/*--------------------------------------------
 
 --------------------------------------------*/
void AtomsMD::getset_pe(PyGetSetDef& getset)
{
    getset.name=(char*)"pe";
    getset.doc=(char*)R"---(
    (double) potential energy
    
    Potential energy
    )---";
    getset.get=[](PyObject* self,void*)->PyObject*
    {
        return var<type0>::build(reinterpret_cast<Object*>(self)->atoms->pe);
    };
    getset.set=[](PyObject* self,PyObject* val,void*)->int
    {
        PyErr_SetString(PyExc_TypeError,"readonly attribute");
        return -1;
    };
}
/*--------------------------------------------*/
#ifdef POTFIT
PyMethodDef AtomsMD::methods[]=EmptyPyMethodDef(17);
#else
PyMethodDef AtomsMD::methods[]=EmptyPyMethodDef(16);
#endif
/*--------------------------------------------*/
void AtomsMD::setup_tp_methods()
{
    ml_do(methods[0]);
    ml_mul(methods[1]);
    ml_cell_change(methods[2]);
    ml_autogrid(methods[3]);
    ml_strain(methods[4]);
    ml_create_temp(methods[5]);
    ml_add_elem(methods[6]);
    ForceFieldLJ::ml_new(methods[7]);
    ForceFieldEAM::ml_new(methods[8],methods[9],methods[10]);
    ForceFieldFS::ml_new(methods[11]);
    ForceFieldEAMFunc::ml_new(methods[12]);
    ImportCFGMD::ml_import(methods[13]);
    BCTPolarity::ml_bct_polarity(methods[14]);
#ifdef POTFIT
    ForceFieldEAMPotFitAckOgata::ml_new(methods[15]);
#endif
}
/*--------------------------------------------
 
 --------------------------------------------*/
void AtomsMD::ml_create_temp(PyMethodDef& tp_method)
{
    tp_method.ml_flags=METH_VARARGS | METH_KEYWORDS;
    tp_method.ml_name="create_temp";
    tp_method.ml_meth=(PyCFunction)(PyCFunctionWithKeywords)(
    [](PyObject* self,PyObject* args,PyObject* kwds)->PyObject*
    {
        AtomsMD::Object* __self=reinterpret_cast<AtomsMD::Object*>(self);
        if(std::isnan(__self->atoms->kB))
        {
            PyErr_SetString(PyExc_TypeError,"boltzmann constant should be set prior to create_temp");
            return NULL;
        }
        
        FuncAPI<type0,int> f("create_temp",{"temp","seed"});
        f.logics<0>()[0]=VLogics("gt",0.0);
        f.logics<1>()[0]=VLogics("gt",0);
        if(f(args,kwds)) return NULL;
        
        __self->atoms->create_T(f.val<0>(),f.val<1>());
        Py_RETURN_NONE;
    });
    tp_method.ml_doc=R"---(
    create_temp(temp,seed)
    
    Create a random velcity field
        
    Parameters
    ----------
    temp : double
       Temperature
    seed : int
       random seed
    
    Returns
    -------
    None
   
    )---";
}
/*--------------------------------------------
 
 --------------------------------------------*/
void AtomsMD::ml_add_elem(PyMethodDef& tp_method)
{
    tp_method.ml_flags=METH_VARARGS | METH_KEYWORDS;
    tp_method.ml_name="add_elem";
    tp_method.ml_meth=(PyCFunction)(PyCFunctionWithKeywords)(
    [](PyObject* self,PyObject* args,PyObject* kwds)->PyObject*
    {
        FuncAPI<std::string,type0> f("add_elem",{"elem","mass"});
        f.logics<1>()[0]=VLogics("gt",0.0);
        if(f(args,kwds)) return NULL;
        
        AtomsMD::Object* __self=reinterpret_cast<AtomsMD::Object*>(self);
        __self->atoms->elements.add_type(f.val<1>(),f.val<0>().c_str());
        
        Py_RETURN_NONE;
    });
    tp_method.ml_doc=R"---(
    add_elem(elem,mass)
    
    Add a new element to the system
        
    Parameters
    ----------
    elem : string
       New element
    seed : double
       Mass of the element
    
    Returns
    -------
    None
   
    )---";
}
/*--------------------------------------------
 
 --------------------------------------------*/
void AtomsMD::ml_rdf(PyMethodDef& tp_method)
{
    tp_method.ml_flags=METH_VARARGS | METH_KEYWORDS;
    tp_method.ml_name="rdf";
    tp_method.ml_meth=(PyCFunction)(PyCFunctionWithKeywords)(
    [](PyObject* self,PyObject* args,PyObject* kwds)->PyObject*
    {
        AtomsMD::Object* __self=reinterpret_cast<AtomsMD::Object*>(self);
 
        FuncAPI<symm<type0**>,int,std::string*> f("rdf",{"r_c","N","elems"});
        f.noptionals=1;
        f.logics<0>()[0]=VLogics("ge",0.0);
        f.logics<1>()[0]=VLogics("gt",1);
 
        const std::string* names=__self->atoms->elements.names;
        const size_t nelems=__self->atoms->elements.nelems;
        if(f(args,kwds)) return NULL;
        if(f.remap<2,0>("elements present in system",names,nelems))
            return NULL;
 

        Py_RETURN_NONE;
    });

    tp_method.ml_doc=R"---(
    NONE
    )---";

}

