struct ATStackType;

/* tags for types of single values */
typedef enum ATVTypeTag {
    var_vtype,          // an unbound variable
    scalarvar_vtype,    // ?? mystery?
    func_vtype,         // a function (w/ two child stack types)
    list_vtype,         // a list (of... some value type)
    basetype_vtype,     // a real type's type, you know (specified num, char, string, etc)
    product_vtype,      // a product type (i.e. a pair of two value types)
    unified_vtype,      // unified so forwarding address of where to check instead
    error_vtype,        // an error :(
} ATVTypeTag;

typedef struct ATValueType {
    ATVTypeTag tag;
    long int id;
    union {
        char var_name;
        struct {
            struct ATStackType *in;
            struct ATStackType *out;
        } func_type;
        struct {
            struct ATValueType *left;
            struct ATValueType *right;
        } prod_type;
        struct ATValueType *v;
        char *type_name;
        struct error *err;
    } content;
    int refs;
} ATValueType;

ATValueType *vt_num;
ATValueType *vt_bool;
ATValueType *vt_char;

// tag for types of stacks
typedef enum ATSTypeTag {
    var_stype,          // an unbound stack variable 'X, 'Y, 'Z...
    toptype_stype,      // some other stack, with a vtype on top (like a linked list)
    unified_stype,      // unified -- check somewhere else instead
    zero_stype,         // zero -- only unifies with plain variables to prevent stack muffing
    error_stype,        // error :(
} ATSTypeTag;

typedef struct ATStackType {
    ATSTypeTag tag;
    long int id;
    union {
        char var_name;
        struct {
            ATValueType *top;
            struct ATStackType *rest;
        } top_type;
        struct ATStackType *unif;
        struct error *err;
    } content;
    int refs;
} ATStackType;
