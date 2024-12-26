
class ExpressionAtom
{

};

struct Double: public ExpressionAtom
{
    double value;
};

template <typename T>
T & as(const ExpressionAtom & atom){
    
}