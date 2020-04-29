#include <iostream>

class Mtl
{
  public:
    std::string name;
    float scalaru;
    float scalarv;
    float scalarw;
    float posu;
    float posv;
    float posw;
    float rangebase;
    float rangegain;

    Mtl()
    {
    }

    Mtl(std::string n, float us, float vs, float ws, float uo, float vo, float wo, float base, float gain)
    {
      name = n;
      scalaru = us;
      scalarv = vs;
      scalarw = ws;
      posu = uo;
      posv = vo;
      posw = wo;
      rangebase = base;
      rangegain = gain;
    }
};

std::ostream& operator<<(std::ostream& os, const Mtl& mtl)
{
    os << mtl.name << " scalar [" << mtl.scalaru << ", " << mtl.scalarv << ", " << mtl.scalarw << "]" << " position [" << mtl.posu << ", " << mtl.posv << ", " << mtl.posw << "]" << " scalar range [" << mtl.rangebase << ", " << mtl.rangegain << "]" << std::endl;
    return os;
}
