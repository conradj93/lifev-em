/*
 * EMMaterialData.hpp
 *
 *  Created on: 29/apr/2014
 *      Author: srossi
 */

#ifndef EMACTIVESTRESSTYPE_HPP_
#define EMACTIVESTRESSTYPE_HPP_

#include <lifev/em/solver/mechanics/materials/functions/FunctionsList.hpp>

namespace LifeV
{

template<typename Mesh>
class EMActiveMaterialType
{
public:

    typedef typename boost::shared_ptr<MaterialFunctions::EMMaterialFunctions<Mesh> >    materialFunctionsPtr_Type;
    typedef std::vector<materialFunctionsPtr_Type>  vectorMaterialsPtr_Type;

    typedef ETFESpace< mesh_Type, MapEpetra, 3, 1 >                        scalarETFESpace_Type;
    typedef boost::shared_ptr<ETFESpace< mesh_Type, MapEpetra, 3, 1 > >    scalarETFESpacePtr_Type;

    typedef FactorySingleton<Factory<EMActiveMaterialType<Mesh>, std::string> >  EMActiveStressFactory;


    typedef Mesh mesh_Type;
    //template <class Mesh>
    using ETFESpacePtr_Type = boost::shared_ptr<ETFESpace<Mesh, MapEpetra, 3, 3 > >;
    //template <class Mesh>
    using FESpacePtr_Type = boost::shared_ptr< FESpace< Mesh, MapEpetra >  >;

    typedef VectorEpetra           vector_Type;
    typedef boost::shared_ptr<vector_Type>         vectorPtr_Type;

    typedef MatrixEpetra<Real>           matrix_Type;
    typedef boost::shared_ptr<matrix_Type>         matrixPtr_Type;


    EMActiveMaterialType (std::string materialName = "None", UInt n = 0);
    virtual ~EMActiveMaterialType() {}


    inline std::string& materialName()
    {
        return M_materialName;
    }

    inline vectorMaterialsPtr_Type materialFunctionList()
    {
        return M_materialFunctionList;
    }

    void
    computeJacobian ( const vector_Type&       disp,
                      const vector_Type&       activation,
                      scalarETFESpacePtr_Type  aETFESpace,
                      ETFESpacePtr_Type        dispETFESpace,
                      FESpacePtr_Type          dispFESpace,
                      matrixPtr_Type           jacobianPtr);
    //



    void
    computeResidual ( const vector_Type&       disp,
                      const vector_Type&       activation,
                      scalarETFESpacePtr_Type  aETFESpace,
                      ETFESpacePtr_Type        dispETFESpace,
                      FESpacePtr_Type          dispFESpace,
                      vectorPtr_Type           residualVectorPtr);

    inline void showMe()
    {
        std::cout << "Material Type: " << M_materialName << "\n";
    }

protected:
    std::string M_materialName;
    vectorMaterialsPtr_Type M_materialFunctionList;
    vectorPtr_Type          M_activeStressPtr;

};


template<typename Mesh>
EMActiveMaterialType<Mesh>::EMActiveMaterialType (std::string materialName, UInt n ) :
    M_materialName (materialName),
    M_materialFunctionList (n)
{

}



template<typename Mesh>
void
EMActiveMaterialType<Mesh>::computeJacobian ( const vector_Type&       disp,
                                              const vector_Type&       activation,
                                              scalarETFESpacePtr_Type  aETFESpace,
                                              ETFESpacePtr_Type        dispETFESpace,
                                              FESpacePtr_Type          dispFESpace,
                                              matrixPtr_Type           jacobianPtr)
{
    int n = M_materialFunctionList.size();
    for (int j (0); j < n; j++)
    {
        M_materialFunctionList[j]->computeJacobian (disp, activation, aETFESpace, dispETFESpace, dispFESpace, jacobianPtr);
    }
}

template <typename Mesh>
void
EMActiveMaterialType<Mesh>::computeResidual ( const vector_Type&       disp,
                                              const vector_Type&       activation,
                                              scalarETFESpacePtr_Type  aETFESpace,
                                              ETFESpacePtr_Type        dispETFESpace,
                                              FESpacePtr_Type          dispFESpace,
                                              vectorPtr_Type           residualVectorPtr)
{
    int n = M_materialFunctionList.size();
    for (int j (0); j < n; j++)
    {
        M_materialFunctionList[j]->computeResidual (disp, activation, aETFESpace, dispETFESpace, dispFESpace, residualVectorPtr);
    }
}



}//LifeV
#endif /* EMMATERIALDATA_HPP_ */