#ifndef __MAPP__md_muvt__
#define __MAPP__md_muvt__
#include "md_nvt.h"
namespace MAPP_NS
{
    class MDMuVT:public MDNVT
    {
    private:
        int seed;
        type0 mu;
        std::string gas_elem_name;
        elem_type gas_elem;
        int nevery;
        int nattempts;
    protected:
        void update_x_d__x(type0);
        void pre_run_chk(AtomsMD*,ForceFieldMD*);
    public:
        MDMuVT(type0,type0,type0,std::string,int);
        ~MDMuVT();
        void run(int);
        
        
        typedef struct
        {
            PyObject_HEAD
            MDMuVT* md;
        }Object;
        
        static PyTypeObject TypeObject;
        static PyObject* __new__(PyTypeObject*,PyObject*, PyObject*);
        static int __init__(PyObject*, PyObject*,PyObject*);
        static PyObject* __alloc__(PyTypeObject*,Py_ssize_t);
        static void __dealloc__(PyObject*);
        
        
        static PyMethodDef methods[];
        static void setup_tp_methods();
        static void ml_run(PyMethodDef&);
        
        
        static PyGetSetDef getset[];
        static void setup_tp_getset();
        static void getset_nevery(PyGetSetDef&);
        static void getset_nattempts(PyGetSetDef&);
        static void getset_seed(PyGetSetDef&);
        static void getset_gas_element(PyGetSetDef&);
        
        
        static void setup_tp();
        
    };
}
#endif
