//  composite FESpace
#ifndef COMPOSITE_FESPACE_HPP_
#define COMPOSITE_FESPACE_HPP_

//#include "HashMatrix.hpp"
//#include "lgmat.hpp"

#ifndef FFLANG
#ifdef PARALLELE

#define BOOST_NO_CXX17_IF_CONSTEXPR
#include <ff++.hpp>
#include <AFunction_ext.hpp>
#include <lgfem.hpp>
#include <R3.hpp>

#include <htool/htool.hpp>

// include the bemtool library .... path define in where library
//#include <bemtool/operator/block_op.hpp>
#include <bemtool/tools.hpp>
#include <bemtool/fem/dof.hpp>
#include <bemtool/operator/operator.hpp>
#include <bemtool/miscellaneous/htool_wrap.hpp>
//#include "PlotStream.hpp"
#include "HashMatrix.hpp"
#include "common.hpp"

//extern FILE *ThePlotStream;

using namespace std;
//using namespace htool;
//using namespace bemtool;

#include <type_traits>

//typedef   LinearComb<MGauche,C_F0> Finconnue;
//typedef   LinearComb<MDroit,C_F0> Ftest;
//typedef  const Finconnue  finconnue;
//typedef  const Ftest ftest;
//class CDomainOfIntegration;
//class FormBilinear;
#include "bem.hpp"

#endif

#endif

template<class R>  //  to make   A=linearform(x)
struct OpMatrixtoBilinearFormVG
  : public OneOperator
{
  typedef typename Call_FormBilinear<vect_generic_v_fes,vect_generic_v_fes>::const_iterator const_iterator;
  int init;
  
  class Op : public E_F0mps {
    public:
      Call_FormBilinear<vect_generic_v_fes,vect_generic_v_fes> *b;
      Expression a;
      int init;
      //AnyType operator()(Stack s)  const;
      
      Op(Expression aa,Expression  bb,int initt)
        : b(new Call_FormBilinear<vect_generic_v_fes,vect_generic_v_fes>(* dynamic_cast<const Call_FormBilinear<vect_generic_v_fes,vect_generic_v_fes> *>(bb))),a(aa),init(initt)
    { 
      assert(b && b->nargs);
      bool iscmplx=FieldOfForm(b->largs,IsComplexType<R>::value)  ;
        // cout<< "FieldOfForm:iscmplx " << iscmplx << " " << IsComplexType<R>::value << " " << ((iscmplx) == IsComplexType<R>::value) << endl;
      ffassert( (iscmplx) == IsComplexType<R>::value);
    }
    operator aType () const { return atype<Matrice_Creuse<R>  *>();}

    AnyType operator()(Stack s)  const;
  };

  E_F0 * code(const basicAC_F0 & args) const
  { return  new Op(to<Matrice_Creuse<R>*>(args[0]),args[1],init); }
  OpMatrixtoBilinearFormVG(int initt=0) :
    OneOperator(atype<Matrice_Creuse<R>*>(),atype<Matrice_Creuse<R>*>(),atype<const Call_FormBilinear<vect_generic_v_fes,vect_generic_v_fes>*>()),
    init(initt){};

};


inline string typeFEtoString(int typeFE)
{
  string toto="";
  if(typeFE == 2){
    toto= "FESpace2D Vol";
  }else if(typeFE == 3){
    toto= "FESpace3D Vol";
  }else if(typeFE == 4){
    toto= "FESpace3D Surf";
  }else if(typeFE == 5){
    toto= "FESpace3D Curve";
  }
  else{
    cerr << "func typeFEtoString ::  error in the type of FESpace " << endl;
    ffassert(0);
  }
  return toto;
}


/**
       *  @brief  Builds a new largs  whit each element are included in one block
       *  @param  largs list of argument of the initial Forms 
       *  @param  NpUh  number of FESpace in UH
       *  @param  NpVh  number of FESpace in Vh
       *  @param  indexBlockUh give the index of the block for a given component of FESpace Uh  
       *  @param  indexBlockVh give the index of the block for a given component of FESpace Vh  
       * 
       */

inline list<C_F0>  creationLargsForCompositeFESpace( const list<C_F0> & largs, const int &NpUh, const int &NpVh, 
                                        const KN<int> &indexBlockUh, const KN<int> &indexBlockVh ){
  
  // At the end of this function, every element of newlargs is included in one block
  list<C_F0> newlargs; // creation de la nouvelle list largs

  // const_iterator
  list<C_F0>::const_iterator ii,ib=largs.begin(),ie=largs.end(); 
  // loop over largs information 
  cout << "loop over the integral" << endl;

  int count_integral = 0;
  for (ii=ib;ii != ie;ii++) {
    count_integral++;
    if(verbosity>0){
      cout <<"========================================================" << endl;
      cout <<"=                                                      =" << endl;
      cout << "reading the " << count_integral << "-th integral" << endl;
    }
    Expression e=ii->LeftValue();
    aType r = ii->left();
    if(verbosity>0){
      cout << "e=" << e << ", " << "r=" << r << endl;
      cout <<"=                                                      =" << endl;
    }

    // ***************************************
    // Case FormBillinear
    // ***************************************
    if (r==atype<const  FormBilinear *>() ){
      const FormBilinear * bb=dynamic_cast<const  FormBilinear *>(e);
      const CDomainOfIntegration & di= *bb->di;

      BilinearOperator * Op=const_cast<  BilinearOperator *>(bb->b);
      if (Op == NULL) {
        if(mpirank == 0) cout << "dynamic_cast error" << endl; 
        ffassert(0);
      }
      
      size_t Opsize= Op->v.size();
      if( verbosity > 0 ){
        cout << " loop over the term inside the integral" << endl;
        cout << " Number of term in the integral:: Op->v.size()=" << Op->v.size() << endl;
      }
      std::vector< std::pair<int,int> > indexBlock(Opsize);

      for(size_t jj=0; jj<Opsize; jj++){
        indexBlock[jj] = std::pair<int,int>( 
          indexBlockUh[ Op->v[jj].first.first.first ],
          indexBlockVh[ Op->v[jj].first.second.first ] );
        /*
        // first inconnue
        indexBlock[jj].first  = indexBlockUh[ Op->v[jj].first.finc.first ]; 
        // second test
        indexBlock[jj].second = indexBlockUh[ Op->v[jj].first.finc.first ];
        */
      }

      // index to check if a integral is defined on multi block

      int countOP=0;
      // loop over the block
      for(int ibloc=0; ibloc<NpUh; ibloc++){
        for(int jbloc=0; jbloc<NpVh; jbloc++){

          BilinearOperator * OpBloc= new BilinearOperator();
          countOP=0;
          for(size_t jj=0; jj<Opsize; jj++){
            if( indexBlock[jj].first == ibloc && indexBlock[jj].second == jbloc){
              if (countOP == 0){
                cout << "OpBloc->v.size()= " << OpBloc->v.size() << endl;
                cout << "OpBloc->v.empty()= "<< OpBloc->v.empty() << endl;
                ffassert( OpBloc->v.empty() );
              } 
              OpBloc->add(Op->v[jj].first, Op->v[jj].second); // Add the billinearOperator to bloc (ibloc,jbloc).
              countOP += 1;
            }
          }
          
          if( countOP > 0 ){   
            cout <<  countOP << " voila titi " << "OpBloc->v.size()= " << OpBloc->v.size() << endl; 
            ffassert( OpBloc->v.size() > 0); 
            
            // FormBilinear *titi = new FormBilinear( &di, OpBloc );
            newlargs.push_back( C_F0( new FormBilinear( &di, OpBloc ), r ) ); 
            

            //newlargs.emplace_back( C_F0( new FormBilinear( &di, OpBloc ), r ) ); 
            //delete titi;
          }
          
          delete OpBloc;
        }
      }
    } // end billinear type
    // ***************************************
    // Case Boundary condition
    // ***************************************
    else if(r == atype<const  BC_set  *>()){
      cout << " BC in variational form " << endl;
      BC_set * bc=dynamic_cast< BC_set *>(e);

      int sizebc=bc->bc.size();
      std::vector< int > indexBlock(sizebc);
      // calculate the index of the componenent where the bloc
      for (int k=0; k<sizebc; k++)
      {
        pair<int,Expression> xx=bc->bc[k];
        indexBlock[k] = indexBlockUh[xx.first];
      }
      bool addBC = false;
      bool *okBC =new bool[NpUh];
      for(int ibloc=0; ibloc<NpUh; ibloc++){
         addBC = false;
        // construction of okBC for ibloc 
        for (int k=0; k<sizebc; k++){
          if(indexBlock[k] == ibloc){
            okBC[k] = true;
            addBC = true;
          }
          else{
            okBC[k] = false;
          }
        }
        // add the BC_set correspond to the ibloc of the composite FESpace 
        if(addBC) newlargs.push_back( C_F0( new BC_set(*bc,okBC), r) ); 
      }
      delete [] okBC;
    }
    #ifndef FFLANG
    #ifdef PARALLELE
    // ******************************************
    // Case BemKFormBilinear (KERNEL FORM ONLY)
    // ******************************************
    else if (r==atype<const BemFormBilinear *>() ){
      BemFormBilinear * bbtmp= dynamic_cast< BemFormBilinear *>(e);
      ffassert(bbtmp);
      int VVFBEM = bbtmp->type;

      if(VVFBEM ==1){
        BemKFormBilinear * bb= dynamic_cast< BemKFormBilinear *>(e);
        ffassert(bb);

        // creation of the new operator
        BemKFormBilinear * bbnew = new BemKFormBilinear( bb->di, FoperatorKBEM(bb->b->kbem, *(bb->b->fi), *(bb->b->ft) ) ); // marche ???
        newlargs.push_back( C_F0( bbnew, r ) );

        // newlargs.push_back( *ii );
      }
      else if(VVFBEM == 2){
        cerr << " BEM Potential in composite FESpace in construction " << endl;
        ffassert(0);
      }
      else{
        cerr << "VFBEM must be egal to 1(kernel) or 2(potential)" << endl;
        ffassert(0);
      }
    }
    #endif
    #endif

  } // end iterator oflargs
  return newlargs;
}

/*
inline list<C_F0>  creationBilinearForm( const list<C_F0> & largs, 
                                        const KN<int> & localIndexInTheBlockUh,
                                        const KN<int> & localIndexInTheBlockVh )
{
  int UhtotalNbItem = localIndexInTheBlockUh.size();
  int VhtotalNbItem = localIndexInTheBlockVh.size();

  // ================================================

  list<C_F0> newlargs; // creation de la nouvelle list largs

  list<C_F0>::const_iterator ii,ib=largs.begin(),ie=largs.end(); 

  // loop over largs information 
  cout << "loop over the integral" << endl;

  int count_integral = 0;
  for (ii=ib;ii != ie;ii++) {
    count_integral++;
    cout <<"========================================================" << endl;
    cout <<"=                                                      =" << endl;
    cout << "reading the " << count_integral << "-th integral" << endl;
    Expression e=ii->LeftValue();
    aType r = ii->left();
    cout << "e=" << e << ", " << "r=" << r << endl;
    cout <<"=                                                      =" << endl;

    // ***************************************
    // Case FormBillinear
    // ***************************************
    if (r==atype<const  FormBilinear *>() ){
      const FormBilinear * bb=dynamic_cast<const  FormBilinear *>(e);
      const CDomainOfIntegration & di= *bb->di;

      cout << "di.kind=" << di.kind << endl;
      cout << "di.dHat=" << di.dHat << endl;
      cout << "di.d=" << di.d << endl;
      cout << "di.Th=" << di.Th << endl;

  
      BilinearOperator * Op=const_cast<  BilinearOperator *>(bb->b);
      if (Op == NULL) {
        if(mpirank == 0) cout << "dynamic_cast error" << endl; 
        ffassert(0);
      }
      
      // creation of the new bilinear operator 
      BilinearOperator * OpChange= new BilinearOperator(*Op); // do a copy of the true BilinearOperator

      size_t Opsize= OpChange->v.size();
      cout << " loop over the term inside the integral" << endl;
      cout << " Number of term in the integral:: Op->v.size()=" << Op->v.size() << endl;

      // index to check if a integral is defined on multi block

      for(size_t jj=0; jj<Opsize; jj++){
        // inconnue
        OpChange->v[jj].first.first.first  = 31+jj;
        // test
        OpChange->v[jj].first.second.first = 32+jj;

        
        // // inconnue
        // OpChange->v[jj].first.first.first  = localIndexInTheBlockUh( OpChange->v[jj].first.first.first ); 

        //// test
        // OpChange->v[jj].first.second.first  = localIndexInTheBlockVh( OpChange->v[jj].first.second.first ); 
        

      }
      // add the FormBilinear in newlargs
      newlargs.push_back( C_F0( new FormBilinear( &di, *OpChange ), r ) ); 
    }
    else if(r == atype<const  BC_set  *>()){
      cout << " BC in variational form " << endl;
      BC_set * bc=dynamic_cast< BC_set *>(e);
      // creation of the new operator
      BC_set * newbc = new BC_set(*bc);

      int kk=newbc->bc.size();
         
      for (int k=0;k<kk;k++)
      {
        //pair<int,Expression> &xx2= newbc->bc[k];
        //xx2.first = localIndexInTheBlockUh(xx2.first);
        
        // newbc->bc[k].first = localIndexInTheBlockUh( newbc->bc[k].first );

        newbc->bc[k].first = 20 + newbc->bc[k].first;
      }
      newlargs.push_back( C_F0( newbc, r ) ); 
    }
    #ifndef FFLANG
    #ifdef PARALLELE
    // ******************************************
    // Case BemKFormBilinear (KERNEL FORM ONLY)
    // ******************************************
    else if (r==atype<const BemFormBilinear *>() ){
      BemFormBilinear * bbtmp= dynamic_cast< BemFormBilinear *>(e);
      ffassert(bbtmp);
      int VVFBEM = bbtmp->type;
      cout << " read index term = "<< count_integral << endl;
      if(VVFBEM ==1){
        BemKFormBilinear * bb= dynamic_cast< BemKFormBilinear *>(e);
        ffassert(bb);

        // creation of the new operator
        //BemKFormBilinear * bbnew = new BemKFormBilinear( *bb); // ne marche pas ???
        BemKFormBilinear * bbnew = new BemKFormBilinear( bb->di, FoperatorKBEM(bb->b->kbem, *(bb->b->fi), *(bb->b->ft) ) ); // marche ???

        FoperatorKBEM * b=const_cast<  FoperatorKBEM *>(bbnew->b);
        if (b == NULL) { if(mpirank == 0) cout << "dynamic_cast error" << endl; ffassert(0);}

        // loop over the index of finconnue
        LOperaG * OpG = const_cast<LOperaG *>(b->fi);
        ffassert( OpG->v.size() == 1);
        size_t Opsize= OpG->v.size();

        for(size_t jjj=0; jjj<Opsize; jjj++){
          OpG->v[jjj].first.first = 50;// localIndexInTheBlockUh( OpG->v[jjj].first.first );
        }

        // Loop over the index of ftest
        LOperaD * OpD = const_cast<LOperaD *>(b->ft);
        ffassert( OpD->v.size() == 1);
        Opsize= OpD->v.size();

        for(size_t jjj=0; jjj<Opsize; jjj++){
          OpD->v[jjj].first.first =  51; //localIndexInTheBlockVh( OpD->v[jjj].first.first );
        }
        newlargs.push_back( C_F0( new BemKFormBilinear( *bbnew), r ) );
        
        
        // newlargs.push_back( 
        //  C_F0( 
        //  new BemKFormBilinear( bbnew->di, FoperatorKBEM(b->kbem, *OpG, *OpD) ), r 
        //  ) 
        //);
         
      }
      else if(VVFBEM == 2){
        cerr << " BEM Potential in composite FESpace in construction " << endl;
        ffassert(0);
      }
      else{
        cerr << "VFBEM must be egal to 1(kernel) or 2(potential)" << endl;
        ffassert(0);
      }
    }
    #endif
    #endif

  }
  return newlargs;
}
*/


inline  KNM< list<C_F0> > computeBlockLargs( const list<C_F0> & largs, const int &NpUh, const int &NpVh, const KN<int> &indexBlockUh, const KN<int> &indexBlockVh ){
  
  // creation of the return value 
  KNM< list<C_F0> > block_largs( (long)NpUh, (long)NpVh );

  long UhtotalNbItem = indexBlockUh.N();
  long VhtotalNbItem = indexBlockVh.N();    

  // impression des information de la composition largs
  list<C_F0>::const_iterator ii,ib=largs.begin(),ie=largs.end(); 

  // necessaire :: UhtotalNbItem, indexBlockUh

  // Loop to put each term of the varf in each correct block

  // loop over largs information 
  cout << "loop over the integral" << endl;

  int count_integral = 0;
  for (ii=ib;ii != ie;ii++) {
    count_integral++;
    cout <<"========================================================" << endl;
    cout <<"=                                                      =" << endl;
    cout << "reading the " << count_integral << "-th term of the variational form used to define the matrix" << endl;
    Expression e=ii->LeftValue();
    aType r = ii->left();
    cout << "e=" << e << ", " << "r=" << r << endl;
    cout <<"=                                                      =" << endl;

    // ***************************************
    // Case FormBillinear
    // ***************************************
    if (r==atype<const  FormBilinear *>() ){
      const FormBilinear * bb=dynamic_cast<const  FormBilinear *>(e);
      const CDomainOfIntegration & di= *bb->di;

      cout << "di.kind=" << di.kind << endl;
      cout << "di.dHat=" << di.dHat << endl;
      cout << "di.d=" << di.d << endl;
      cout << "di.Th=" << di.Th << endl;

      int    d = di.d;
      int dHat = di.dHat;

      // Sert a verifier que "*bb->di->Th" est du bon type ==> A enlever    
      BilinearOperator * Op=const_cast<  BilinearOperator *>(bb->b);
      if (Op == NULL) {
        if(mpirank == 0) cout << "dynamic_cast error" << endl; 
        ffassert(0);
      }
      
      size_t Opsize= Op->v.size();
      cout << " loop over the term inside the integral" << endl;
      cout << " Number of term in the integral:: Op->v.size()=" << Op->v.size() << endl;

    
      // index to check if a integral is defined on multi block
      int indexOfBlockUh = -1; // A changer de nom
      int indexOfBlockVh = -1; // A changer de nom
      for(size_t jj=0; jj<Opsize; jj++){
        // attention la fonction test donne la ligne
        //  et la fonction test est en second
        BilinearOperator::K ll = Op->v[jj];
        pair<int,int> finc(ll.first.first), ftest(ll.first.second);
        cout << " operateur jj= " << jj << endl;
        cout << " FormBilinear: number of unknown finc=" <<  finc.first << " ,ftest= " << ftest.first << endl;
        cout << " FormBilinear: operator order finc   =" << finc.second << " ,ftest= " << ftest.second << endl; // ordre   only op_id=0
        
        // Fred fait peut être un message après ????
        // verification que la taille des tableaux des fonctions tests et de la fonction inconnue``
        // sont correctes.  
        ffassert( -1  < finc.first  && finc.first < UhtotalNbItem);
        ffassert( -1  < ftest.first && ftest.first < VhtotalNbItem);

        // finc.first : index de component de la fonction inconnue
        // ftest.first: index de component de la fonction test
        //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
        // finc.second : renvoie l'index du type de l'operateur: Id, dx(), dy(), dz(), dxx(), dxy(), ...
        //
        // la liste des index des operateurs est definis dans [[????]].
        //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
        // exemple vf([u1,u2,..,u30],[v1,v2,...,v30]) = int2d(Th)(dx(u20)*v15)
        //      finc.first  = 20 , ftest.first = 15
        //      finc.second = 1 , ftest.second = 0

        //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

        if( jj== 0 ){
          indexOfBlockUh = indexBlockUh[finc.first];
          indexOfBlockVh = indexBlockVh[ftest.first];
        }
        else if( indexOfBlockUh != indexBlockUh[finc.first] ){
          cerr << "The " << count_integral <<"-th integral(s) contains the constribution of two different blocks:" << endl;
          cerr << "the first term correspond to block (" << indexOfBlockUh << " , " <<  indexOfBlockVh << ")" << endl;
          cerr << "the "<<jj <<"-th term correspond to block (" << indexBlockUh(finc.first) << " " <<  indexBlockVh(ftest.first) << ")" << endl;
          cerr << "You need to separate the integral in individual part." << endl;
          cerr << "Remark: scalar product in N dimension correspond to N terms inside the integral" << endl;
          cerr << "A ameliorer Jacques." << endl;
          ffassert(0);
        }
        else if( indexOfBlockVh != indexBlockVh[ftest.first] ){
          cerr << "The " << count_integral <<"-th integral(s) contains the constribution of two different blocks:" << endl;
          cerr << "the first term correspond to block (" << indexOfBlockUh << " , " <<  indexOfBlockVh << ")" <<endl;
          cerr << "the "<<jj <<"-th term correspond to block (" << indexBlockUh(finc.first) << ", " <<  indexBlockVh(ftest.first) << ")" << endl;
          cerr << "You need to separate the integral in individual part." << endl;
          cerr << "Remark: scalar product in N dimension correspond to N terms inside the integral" << endl;
          cerr << "A ameliorer Jacques." << endl;
          ffassert(0);
        }

        ffassert( indexOfBlockUh == indexBlockUh(finc.first) );
        ffassert( indexOfBlockVh == indexBlockVh(ftest.first) );
      }

    ffassert( indexOfBlockUh >= 0 && indexOfBlockVh >= 0);
    
    // A faire :: recuperation des éléments pour chacun des blocs
    // Actuellement, on associe une intégrale par block ==> 
    
    block_largs(indexOfBlockUh,indexOfBlockVh).push_back(*ii);

    cout << "The " << count_integral <<"-th integral(s) is added to the block (" << indexOfBlockUh << " , " <<  indexOfBlockVh << ")" <<endl;

    }
#ifndef FFLANG
#ifdef PARALLELE
    // ******************************************
    // Case BemKFormBilinear (KERNEL FORM ONLY)
    // ******************************************
    else if (r==atype<const BemFormBilinear *>() ){
      BemFormBilinear * bbtmp= dynamic_cast< BemFormBilinear *>(e);
      int VVFBEM = bbtmp->type;

      if(VVFBEM ==1){
        //BemKFormBilinear * bb=new BemKFormBilinear(*dynamic_cast<const BemKFormBilinear *>(e));
        const BemKFormBilinear * bb= dynamic_cast<const BemKFormBilinear *>(e);
        FoperatorKBEM * b=const_cast<  FoperatorKBEM *>(bb->b);
        if (b == NULL) { if(mpirank == 0) cout << "dynamic_cast error" << endl; exit(0);}

        int indexOfBlockUh = -1; // A changer de nom
        int indexOfBlockVh = -1; // A changer de nom


        // loop over the index of finconnue
        LOperaG * OpG = const_cast<LOperaG *>(b->fi);
        ffassert( OpG->v.size() == 1);
        size_t jj =0;
        for (LOperaG::const_iterator lop=OpG->v.begin();lop!=OpG->v.end();lop++){

          LOperaG::K lf(*lop);
          pair<int,int> finc(lf.first);
          cout << " operateur jj= " << jj << endl;
          cout << " BemFormLinear: number of unknown finc= " << finc.first << endl;
          ffassert( -1  < finc.first && finc.first < UhtotalNbItem);     // check the index 

          if( jj== 0 ){
            indexOfBlockUh = indexBlockUh[finc.first];
          }
          else if( indexOfBlockUh != indexBlockUh[finc.first] ){
            cerr << "The " << count_integral <<"-th term of the varitional form contains the constribution of two different FESpace:" << endl;
            cerr << "This terms correspond to a BEM integral terms" << endl;
            cerr << "the first term correspond to element " << indexOfBlockUh << " of the Composite FESpace (Finconnu)." << endl;
            cerr << "the "<< jj <<"-th term correspond to element " << indexBlockUh(finc.first) << endl;
            cerr << "In a composite FESpace, you need to define a BEM integral for each FESpace individually." << endl;
            cerr << "A ameliorer Jacques." << endl;
            ffassert(0);
          }
          jj+=1;
        }


        // Loop over the index of ftest
        LOperaD * OpD = const_cast<LOperaD *>(b->ft);
        ffassert( OpD->v.size() == 1);
        jj =0; // reinitialisation ton zero
        for (LOperaD::const_iterator lop=OpD->v.begin();lop!=OpD->v.end();lop++){
          
          LOperaD::K lf(*lop);
          pair<int,int> ftest(lf.first);
          cout << " operateur jj= " << jj << endl;
          cout << " BemFormLinear: number of unknown ftest= " << ftest.first << endl;
          ffassert( -1  < ftest.first && ftest.first < VhtotalNbItem);    // check the index 

          if( jj== 0 ){
            indexOfBlockVh = indexBlockVh[ftest.first];
          }
          else if( indexOfBlockVh != indexBlockVh[ftest.first] ){
            cerr << "The " << count_integral <<"-th term of the varitional form contains the constribution of two different FESpace:" << endl;
            cerr << "This terms correspond to a BEM integral terms" << endl;
            cerr << "the first term correspond to element " << indexOfBlockVh << " of the Composite FESpace (Ftest)." << endl;
            cerr << "the "<< jj <<"-th term correspond to element " << indexBlockVh(ftest.first) << endl;
            cerr << "In a composite FESpace, you need to define a BEM integral term for each FESpace individually." << endl;
            cerr << "A ameliorer Jacques." << endl;
            ffassert(0);
          }
          jj+=1;
        }
        block_largs(indexOfBlockUh,indexOfBlockVh).push_back(*ii); 

      }else if(VVFBEM == 2){
        BemPFormBilinear * bb=new BemPFormBilinear(*dynamic_cast<const BemPFormBilinear *>(e));
        FoperatorPBEM * b=const_cast<  FoperatorPBEM *>(bb->b);
        if (b == NULL) { if(mpirank == 0) cout << "dynamic_cast error" << endl; }

        cerr << " BEM Potential in composite FESpace in construction " << endl;
        ffassert(0);
      }

    }
#endif
#endif
    else if(r == atype<const  BC_set  *>()){
      cout << " BC in variational form " << endl;
      
      const BC_set * bc=dynamic_cast<const  BC_set *>(e);
      
      // index to check if a integral is defined on multi block
      int indexOfBlockUh = -1;
    
      int kk=bc->bc.size();
      for (int k=0;k<kk;k++)
      {
          pair<int,Expression> xx=bc->bc[k];
          ffassert( -1  < xx.first  && xx.first < UhtotalNbItem); // check the value of index of the component of the varf
        
          if( k == 0) indexOfBlockUh = indexBlockUh[xx.first]; // index of the block Uh
          else if( indexOfBlockUh != indexBlockUh[xx.first] ){
            cerr << "The " << count_integral <<"-th term of the varitional form contains the constribution of two different FESpace:" << endl;
            cerr << "This terms correspond to Boundary condition" << endl;
            cerr << "the first term correspond to element " << indexOfBlockUh << " of the Composite FESpace " << endl;
            cerr << "the "<<kk <<"-th term correspond to element " << indexBlockUh(xx.first) << endl;
            cerr << "In a composite FESpace, you need to define a BC for each FESpace individually." << endl;
            cerr << "A ameliorer Jacques." << endl;
            ffassert(0);
          }
      }
      // Added the boundary condition in the largs block
      block_largs(indexOfBlockUh,indexOfBlockUh).push_back(*ii); 
        
      //ffassert(0);
    }
    else{
      
      cerr << "Composite FESpace only :: bilinear form" << endl;
      cerr << "                       :: BC" << endl;
      #ifndef FFLANG
      #ifdef PARALLELE
      cerr << "                       :: BemFormBilinear" << endl;
      #endif
      #endif
      ffassert(0);
    }
  }
  return block_largs;
}


// Info necessaire :: " block_largs, localIndexInTheBlockUh, localIndexInTheBlockVh, NpUh, NpVh  
inline void changeComponentFormCompositeFESpace( const KN<int> &localIndexInTheBlockUh, const KN<int> &localIndexInTheBlockVh, 
        KNM< list<C_F0> > & block_largs ){
  // put the right number of each component of each block

  long NpUh = block_largs.N();
  long NpVh = block_largs.M();

  for( long i=0; i<NpUh; i++){
      for( long j=0; j<NpVh; j++){
        
        const list<C_F0> *b_largs=&block_largs(i,j); 
        list<C_F0>::const_iterator b_ii,b_ib=b_largs->begin(),b_ie=b_largs->end(); 
        for (b_ii=b_ib;b_ii != b_ie;b_ii++){
          Expression e=b_ii->LeftValue();
          aType r = b_ii->left();
          // Case FormBilinear
          if (r==atype<const  FormBilinear *>() ){
            const FormBilinear * bb=dynamic_cast<const  FormBilinear *>(e);
      
            BilinearOperator * Op=const_cast<  BilinearOperator *>(bb->b);
            if (Op == NULL) {
              if(mpirank == 0) cout << "dynamic_cast error" << endl; 
            ffassert(0);
            }
      
            size_t Opsize= Op->v.size();
            cout << " loop over the term inside the integral" << endl;
            cout << " Number of term in the integral:: Op->v.size()=" << Op->v.size() << endl;
     
            KN<size_t> index_operator_finc(Opsize);
            KN<int>    new_index_funct_finc(Opsize);

            KN<size_t> index_operator_ftest(Opsize);
            KN<int>    new_index_funct_ftest(Opsize);

            // index to check if a integral is defined on multi block
            for(size_t jj=0; jj<Opsize; jj++){
              // attention la fonction test donne la ligne
              //  et la fonction test est en second
              BilinearOperator::K ll = Op->v[jj];
              pair<int,int> finc(ll.first.first), ftest(ll.first.second);

              long jj2= jj;

              index_operator_finc[ jj2] = jj;
              new_index_funct_finc[ jj2] = localIndexInTheBlockUh(finc.first);
            
              index_operator_ftest[ jj2]  = jj;
              new_index_funct_ftest[ jj2] = localIndexInTheBlockVh(ftest.first);

            }
            changeIndexFunctionInconnue(*Op, index_operator_finc, new_index_funct_finc );
      
            changeIndexFunctionTest(*Op, index_operator_ftest, new_index_funct_ftest  );          
          }  

          #ifndef FFLANG
          #ifdef PARALLELE
          // ******************************************
          // Case BemKFormBilinear (KERNEL FORM ONLY)
          // ******************************************
          else if (r==atype<const BemFormBilinear *>() ){
            BemFormBilinear * bbtmp= dynamic_cast< BemFormBilinear *>(e);
            int VVFBEM = bbtmp->type;

            if(VVFBEM ==1){
              //BemKFormBilinear * bb=new BemKFormBilinear(*dynamic_cast<const BemKFormBilinear *>(e));
              const BemKFormBilinear * bb= dynamic_cast<const BemKFormBilinear *>(e);
              FoperatorKBEM * b=const_cast<  FoperatorKBEM *>(bb->b);
              if (b == NULL) { if(mpirank == 0) cout << "dynamic_cast error" << endl; exit(0);}

              // loop over the index of finconnue
              LOperaG * OpG = const_cast<LOperaG *>(b->fi);
              ffassert( OpG->v.size() == 1);
              size_t Opsize= OpG->v.size();

              for(size_t jjj=0; jjj<Opsize; jjj++){
                LOperaG::K lf=OpG->v[jjj];
                OpG->v[jjj].first.first = localIndexInTheBlockUh( OpG->v[jjj].first.first );
              
                pair<int,int> finc(lf.first);
                cout << " new value :: block i,j=" << i << ","<< j << ", operateur jj= " << jjj << endl;
                cout << " BemormBilinear: number of unknown finc = " << finc.first << endl;
                cout << " BemFormBilinear: operator order   finc = " << finc.second << endl; 
              }


              // Loop over the index of ftest
              LOperaD * OpD = const_cast<LOperaD *>(b->ft);
              ffassert( OpD->v.size() == 1);
              Opsize= OpD->v.size();
              for(size_t jjj=0; jjj<Opsize; jjj++){
                LOperaD::K lf=OpD->v[jjj];
                OpD->v[jjj].first.first = localIndexInTheBlockVh( OpD->v[jjj].first.first );
              
                pair<int,int> ftest(lf.first);
                cout << " new value :: block i,j=" << i << ","<< j << ", operateur jj= " << jjj << endl;
                cout << " BemormBilinear: number of unknown ftest = " << ftest.first << endl;
                cout << " BemFormBilinear: operator order   ftest = " << ftest.second << endl; 
              }
            }
            else if(VVFBEM == 2){
              BemPFormBilinear * bb=new BemPFormBilinear(*dynamic_cast<const BemPFormBilinear *>(e));
              FoperatorPBEM * b=const_cast<  FoperatorPBEM *>(bb->b);
              if (b == NULL) { if(mpirank == 0) cout << "dynamic_cast error" << endl; }

              cerr << " BEM Potential in composite FESpace in construction " << endl;
              ffassert(0);
            }

          }
          #endif
          #endif

          // case BC_set 
          else if(r == atype<const  BC_set  *>()){
            ffassert( i == j ); // diagonal block 
            BC_set * bc=dynamic_cast< BC_set *>(e); // on ne peut pas utiliser " const BC_set * " ou autrement erreur ce ompilation:  Morice

            //KN<int>  new_index_funct_finc( bc.size() );
            int kk=bc->bc.size();
            //pair<int,Expression>  &bc_ib(bc->bc.begin());
            
            for (int k=0;k<kk;k++)
            {
              pair<int,Expression> &xx2= bc->bc[k];
              //new_index_funct_finc[k] = localIndexInTheBlockUh(bc[k].first);
              // change the index of the component to correspond to the index in the block
              xx2.first = localIndexInTheBlockUh(xx2.first);
              //bc->changeNumberOfComponent(k,localIndexInTheBlockUh(xx.first));
              //bc->bc[k].first = localIndexInTheBlockUh( bc->bc[k].first );
            }
          }
        }
        // listOfComponentBilinearForm(*b_largs);
      }
  }
}


inline void reverseChangeComponentFormCompositeFESpace(const KN<int>  &beginBlockUh, const KN<int> &beginBlockVh, 
          KNM< list<C_F0> > & block_largs){
  // Info necessaire :: " block_largs, beginBlockUh, beginBlockVh, NpUh, NpVh  

  long NpUh = block_largs.N();
  long NpVh = block_largs.M();

  // put the right number of component of each block
  for( int i=0; i<NpUh; i++){
      for( int j=0; j<NpVh; j++){
        
        const list<C_F0> *b_largs=&block_largs(i,j); 
        list<C_F0>::const_iterator b_ii,b_ib=b_largs->begin(),b_ie=b_largs->end(); 
        for (b_ii=b_ib;b_ii != b_ie;b_ii++){
          Expression e=b_ii->LeftValue();
          aType r = b_ii->left();

          // bilinear case
          if (r==atype<const  FormBilinear *>() ){
            const FormBilinear * bb=dynamic_cast<const  FormBilinear *>(e);
      
            BilinearOperator * Op=const_cast<  BilinearOperator *>(bb->b);
            if (Op == NULL) {
              if(mpirank == 0) cout << "dynamic_cast error" << endl; 
            ffassert(0);
            }
      
            size_t Opsize= Op->v.size();
      
            KN<size_t> index_operator_finc(Opsize);
            KN<int>    new_index_funct_finc(Opsize);

            KN<size_t> index_operator_ftest(Opsize);
            KN<int>    new_index_funct_ftest(Opsize);

            // index to check if a integral is defined on multi block
            for(size_t jj=0; jj<Opsize; jj++){
              // attention la fonction test donne la ligne
              //  et la fonction test est en second
              BilinearOperator::K ll = Op->v[jj];
              pair<int,int> finc(ll.first.first), ftest(ll.first.second);

              long jj2= jj;

              index_operator_finc[ jj2] = jj;
              new_index_funct_finc[ jj2] = beginBlockUh[i]+finc.first;
            
              index_operator_ftest[ jj2]  = jj;
              new_index_funct_ftest[ jj2] = beginBlockVh[j]+ftest.first; 
            }
            changeIndexFunctionInconnue(*Op, index_operator_finc, new_index_funct_finc );
      
            changeIndexFunctionTest(*Op, index_operator_ftest, new_index_funct_ftest  );          
          }  
      
#ifndef FFLANG
#ifdef PARALLELE
          // ******************************************
          // Case BemKFormBilinear (KERNEL FORM ONLY)
          // ******************************************
          else if (r==atype<const BemFormBilinear *>() ){
            BemFormBilinear * bbtmp= dynamic_cast< BemFormBilinear *>(e);
            int VVFBEM = bbtmp->type;

            if(VVFBEM ==1){
              // BemKFormBilinear * bb=new BemKFormBilinear(*dynamic_cast<const BemKFormBilinear *>(e));
              const BemKFormBilinear * bb= dynamic_cast<const BemKFormBilinear *>(e);
              FoperatorKBEM * b=const_cast<  FoperatorKBEM *>(bb->b);
              if (b == NULL) { if(mpirank == 0) cout << "dynamic_cast error" << endl; exit(0);}

              // loop over the index of finconnue
              LOperaG * OpG = const_cast<LOperaG *>(b->fi);
              ffassert( OpG->v.size() == 1);
              size_t Opsize= OpG->v.size();

              for(size_t jjj=0; jjj<Opsize; jjj++){
                LOperaG::K *lf=&(OpG->v[jjj]);
                OpG->v[jjj].first.first += beginBlockUh[i];
                
                //pair<int,int> finc(lf->first);
                //cout << " new value :: block i,j=" << i << ","<< j << ", operateur jj= " << jjj << endl;
                //cout << " BemormBilinear: number of unknown finc = " << finc.first << endl;
                //cout << " BemFormBilinear: operator order   finc = " << finc.second << endl; 
                
              }

              // Loop over the index of ftest
              LOperaD * OpD = const_cast<LOperaD *>(b->ft);
              ffassert( OpD->v.size() == 1);
              Opsize= OpD->v.size();
              for(size_t jjj=0; jjj<Opsize; jjj++){
                LOperaD::K *lf=&(OpD->v[jjj]);
                OpD->v[jjj].first.first += beginBlockVh[j]; 
                
                //pair<int,int> finc(lf->first);
                //cout << " new value :: block i,j=" << i << ","<< j << ", operateur jj= " << jjj << endl;
                //cout << " BemormBilinear: number of unknown ftest = " << ftest.first << endl;
                //cout << " BemFormBilinear: operator order   ftest = " << ftest.second << endl;
                
              }
            }
            else if(VVFBEM == 2){
              BemPFormBilinear * bb=new BemPFormBilinear(*dynamic_cast<const BemPFormBilinear *>(e));
              FoperatorPBEM * b=const_cast<  FoperatorPBEM *>(bb->b);
              if (b == NULL) { if(mpirank == 0) cout << "dynamic_cast error" << endl; }

              cerr << " BEM Potential in composite FESpace in construction " << endl;
              ffassert(0);
            }

          }
#endif
#endif  
          // BC_set
          // case BC_set 
          else if(r == atype<const  BC_set  *>()){
            ffassert( i == j ); // diagonal block 
            BC_set * bc=dynamic_cast<BC_set *>(e);
        
            //KN<int>  new_index_funct_finc( bc.size() );
            int kk=bc->bc.size();
            for (int k=0;k<kk;k++)
            {
              //bc->bc[k].first += beginBlockUh[i];
              pair<int,Expression> &xx=bc->bc[k];
              xx.first += beginBlockUh[i];
            }
          }
        }
      }
  }
}

template<class FESpaceT1,class FESpaceT2>
MatriceMorse<R> * buildInterpolationMatrixT(const FESpaceT1 & Uh,const FESpaceT2 & Vh,void *data);

template< >
MatriceMorse<R> * buildInterpolationMatrixT<FESpaceL,FESpace>(const FESpaceL & Uh,const FESpace & Vh,void *data);


template< class R, class FESpaceT1, class FESpaceT2 >
Matrice_Creuse<R> *  buildMatrixInterpolationForCompositeFESpace(const FESpaceT1 * Uh ,const FESpaceT2 * Vh){
ffassert(Uh);
ffassert(Vh);
int NUh = Uh->N;
int NVh = Vh->N;

cout << "NUh=" << NUh << ", NVh=" << NVh << endl;
Matrice_Creuse<R> * sparse_mat= new Matrice_Creuse<R>();

// Remarque pas de U2Vc pour l'instant
int* data = new int[4 + NUh];
// default value for the interpolation matrix
data[0]=false;         // transpose not
data[1]=(long) op_id;  // get just value
data[2]=false;         // get just value
data[3]=0L;            // get just value

for(int i=0;i<NUh;++i) data[4+i]=i;//

if(verbosity>3){
  for(int i=0;i<NUh;++i)
  {
    cout << "The Uh componante " << i << " -> " << data[4+i] << "  Componante of Vh  " <<endl;
  }
}
for(int i=0;i<NUh;++i){
  if(data[4+i]>=NVh)
  {
    cout << "The Uh componante " << i << " -> " << data[4+i] << " >= " << NVh << " number of Vh Componante " <<endl;
    ExecError("Interpolation incompability between componante ");
  }
}
const FESpaceT1 &rUh = *Uh;
const FESpaceT2 &rVh = *Vh;

MatriceMorse<R>* titi=buildInterpolationMatrixT<FESpaceT1,FESpaceT2>(rUh,rVh,data);

sparse_mat->init();
sparse_mat->typemat=0;//(TypeSolveMat::NONESQUARE); //  none square matrice (morse)
sparse_mat->A.master( titi );	  //  sparse_mat->A.master(new MatriceMorse<R>(*Uh,*Vh,buildInterpolationMatrix,data));
if(verbosity>3){
  cout << "sparse_mat->typemat=" << sparse_mat->typemat << endl;
  cout << "N=" << sparse_mat->A->n << endl;
  cout << "M=" << sparse_mat->A->m << endl;
}
delete [] data;

return sparse_mat;
}

// A enlever ??
inline void listOfComponentBilinearForm(const list<C_F0> & largs){

  list<C_F0>::const_iterator ii,ib=largs.begin(),ie=largs.end(); 

  // loop over largs information 
  cout << "loop over the integral" << endl;

  int count_integral = 0;
  for (ii=ib;ii != ie;ii++) {
    count_integral++;
    cout <<"========================================================" << endl;
    cout <<"=                                                      =" << endl;
    cout << "reading the " << count_integral << "-th integral" << endl;
    Expression e=ii->LeftValue();
    aType r = ii->left();
    cout << "e=" << e << ", " << "r=" << r << endl;
    cout <<"=                                                      =" << endl;

    // ***************************************
    // Case FormBillinear
    // ***************************************
    if (r==atype<const  FormBilinear *>() ){
      const FormBilinear * bb=dynamic_cast<const  FormBilinear *>(e);
      const CDomainOfIntegration & di= *bb->di;

      cout << "di.kind=" << di.kind << endl;
      cout << "di.dHat=" << di.dHat << endl;
      cout << "di.d=" << di.d << endl;
      cout << "di.Th=" << di.Th << endl;

      int    d = di.d;
      int dHat = di.dHat;


      // Sert a verifier que "*bb->di->Th" est du bon type ==> A enlever
      
      // Recuperation du pointeur sur le maillage de l'integrale
      //if(d==2){ // 3d
      //  pmesh Thtest=GetAny<pmesh >((*bb->di->Th)(stack));
      //  cout << "pointeur du Th de l'integrale =" << Thtest << endl;
      //}
      //else if(d==3 && dHat==3){
      //  pmesh3 Thtest=GetAny<pmesh3 >((*bb->di->Th)(stack));
      //  cout << "pointeur du Th de l'integrale =" << Thtest << endl;
      //}
      //else if(d==3 && dHat==2){
      //  pmeshS Thtest=GetAny<pmeshS >((*bb->di->Th)(stack));
      //  cout << "pointeur du Th de l'integrale =" << Thtest << endl;
      //}
      //else if(d==3 && dHat==1){
      //  pmeshL Thtest=GetAny<pmeshL >((*bb->di->Th)(stack));
      //  cout << "pointeur du Th de l'integrale =" << Thtest << endl;
      //}
      //else{ ffassert(0); }// a faire
      
      BilinearOperator * Op=const_cast<  BilinearOperator *>(bb->b);
      if (Op == NULL) {
        if(mpirank == 0) cout << "dynamic_cast error" << endl; 
        ffassert(0);
      }
      
      size_t Opsize= Op->v.size();
      cout << " loop over the term inside the integral" << endl;
      cout << " Number of term in the integral:: Op->v.size()=" << Op->v.size() << endl;

      // index to check if a integral is defined on multi block

      for(size_t jj=0; jj<Opsize; jj++){
        // attention la fonction test donne la ligne
        //  et la fonction test est en second
        BilinearOperator::K ll = Op->v[jj];
        pair<int,int> finc(ll.first.first), ftest(ll.first.second);
        cout << " operateur jj= " << jj << endl;
        cout << " FormBilinear: number of unknown finc=" <<  finc.first << " ,ftest= " << ftest.first << endl;
        cout << " FormBilinear: operator order finc   =" << finc.second << " ,ftest= " << ftest.second << endl; // ordre   only op_id=0
        
        // finc.first : index de component de la fonction inconnue
        // ftest.first: index de component de la fonction test
        //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
        // finc.second : renvoie l'index du type de l'operateur: Id, dx(), dy(), dz(), dxx(), dxy(), ...
        //
        // la liste des index des operateurs est definis dans [[????]].
        //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
        // exemple vf([u1,u2,..,u30],[v1,v2,...,v30]) = int2d(Th)(dx(u20)*v15)
        //      finc.first  = 20 , ftest.first = 15
        //      finc.second = 1 , ftest.second = 0

        //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%       
      }
    }
    else if(r == atype<const  BC_set  *>()){
      cout << " BC in variational form " << endl;
      const BC_set * bc=dynamic_cast<const  BC_set *>(e);
  
      //KN<int>  new_index_funct_finc( bc.size() );
      int kk=bc->bc.size();
      //cout << "bc.size=" << bc->bc.size() << endl;
      for (int k=0;k<kk;k++)
      {
        //new_index_funct_finc[k] = localIndexInTheBlockUh(bc[k].first);
        // change the index of the component to correspond to the index in the block
        cout << "bc->bc["<< k << "].first= " << bc->bc[k].first << endl; 
      }
      //ffassert(0);
    }
    #ifndef FFLANG
    #ifdef PARALLELE
    // ******************************************
    // Case BemKFormBilinear (KERNEL FORM ONLY)
    // ******************************************
    else if (r==atype<const BemFormBilinear *>() ){
      BemFormBilinear * bbtmp= dynamic_cast< BemFormBilinear *>(e);
      int VVFBEM = bbtmp->type;
      cout << " read index term = "<< count_integral << " VVFBEM=" << VVFBEM << endl;
      if(VVFBEM ==1){
        //BemKFormBilinear * bb=new BemKFormBilinear(*dynamic_cast<const BemKFormBilinear *>(e));
        BemKFormBilinear * bb = dynamic_cast< BemKFormBilinear *>(e);
        FoperatorKBEM * b=const_cast<  FoperatorKBEM *>(bb->b);
        if (b == NULL) { if(mpirank == 0) cout << "dynamic_cast error" << endl; ffassert(0);}

        // loop over the index of finconnue
        LOperaG * OpG = const_cast<LOperaG *>(b->fi);
        ffassert( OpG->v.size() == 1);
        size_t Opsize= OpG->v.size();
        cout << " pointeur  " << OpG << endl;
        for(size_t jjj=0; jjj<Opsize; jjj++){
          LOperaG::K *lf=&(OpG->v[jjj]);
          pair<int,int> finc(lf->first);
          cout << " operateur jj= " << jjj << endl;
          cout << " BemFormBilinear: number of unknown finc = " << finc.first << endl;
          cout << " BemFormBilinear: operator order   finc = " << finc.second << endl; 
        }


        // Loop over the index of ftest
        LOperaD * OpD = const_cast<LOperaD *>(b->ft);
        ffassert( OpD->v.size() == 1);
        Opsize= OpD->v.size();
        for(size_t jjj=0; jjj<Opsize; jjj++){
          LOperaD::K *lf=&(OpD->v[jjj]);
          pair<int,int> ftest(lf->first);
          cout << " operateur jj= " << jjj << endl;
          cout << " BemormBilinear: number of unknown ftest = " << ftest.first << endl;
          cout << " BemFormBilinear: operator order   ftest = " << ftest.second << endl; 
        }
      }
      else if(VVFBEM == 2){
        cerr << " BEM Potential in composite FESpace in construction " << endl;
        ffassert(0);
      }
      else{
        cerr << " VFBEM=1 (kernel) or VFBEM=2 (potential) " << endl;
        ffassert(0);
      }
    }
    #endif
    #endif
    else{
      cerr << "listOfComponentBilinearForm :: vectorial FESpace :: bilinear form only " << endl;
      cerr << "      uniquement terme bilineaire + BC " << endl;
      ffassert(0);
    }
  }

}

/**
       *  @brief  determine if we have BEM bilinear operator in a subblock and the type
       *  @param  largs list of argument of the Bilinear Form
       */

/*
  This function give the good result if only if the FESpace inconnu and  FESpace test are scalar FESpace
  due to this check : 
          if( finc.first==0 && ftest.first==0)      // only first component for finc and ftest
*/

int haveBemSubMatrixBlock(const list<C_F0> & largs, int Uh_NbItem, int Vh_NbItem){

  ffassert( Uh_NbItem == 1 && Vh_NbItem == 1);

  // this function is used to know if a block of the matrix contains BEM Operator
  // return the type of Bem Block
  bool haveBemFormBilinear   = false;
  bool haveMassMatrixforBEMTOOL = false; 
  int nbFB=0; 
  int nbBC=0;
  int nbBEM=0;
  // 
  list<C_F0>::const_iterator ii, b_ib=largs.begin(), b_ie=largs.end(); 
  
#ifndef FFLANG
#ifdef PARALLELE
  for( ii=b_ib;ii != b_ie;ii++){
    Expression e=ii->LeftValue();
    aType r = ii->left();

    // ******************************************
    // Case BemKFormBilinear (KERNEL FORM ONLY)
    // ******************************************
    if (r==atype<const BemFormBilinear *>() ){
      haveBemFormBilinear = true;
      nbBEM++;
    }
  }
#endif
#endif
  if( nbBEM > 1 ){
    cerr << "Two BEM operator in a sub-matrix defined by a time product of two FESpace is not allowed" << endl;
    ffassert(0);
  }

  for( ii=b_ib;ii != b_ie;ii++){
    Expression e=ii->LeftValue();
    aType r = ii->left();
  
    // ***************************************
    // Case FormBillinear
    // ***************************************
    if (r==atype<const  FormBilinear *>() ){
      nbFB++;
      //
      const FormBilinear * bb=dynamic_cast<const  FormBilinear *>(e);
      const CDomainOfIntegration & di= *bb->di;
      // check the integration (keyword)

      BilinearOperator * Op=const_cast<  BilinearOperator *>(bb->b);
      if (Op == NULL) { if(mpirank == 0) cout << "dynamic_cast error" << endl; }

      size_t Opsize= Op->v.size();

      for(size_t jj=0; jj<Opsize; jj++){
        // attention la fonction test donne la ligne
        //  et la fonction test est en second
        BilinearOperator::K ll = Op->v[jj]; //  LinearComb<pair<MGauche,MDroit>,C_F0> BilinearOperator;
        pair<int,int> finc(ll.first.first), ftest(ll.first.second);

        // check if we have a mass matrix that we need to add to BEMTOOL 
        // cf bem.hpp: see pair<BemKernel*,double> getBemKernel(Stack stack, const list<C_F0> & largs).
  
        if( finc.first==0 && ftest.first==0)      // only first component for finc and ftest
          if( finc.second==0 && ftest.second==0 ) // only op_id=0
            if( (di.kind == CDomainOfIntegration::int1d && di.dHat==1) || (di.kind == CDomainOfIntegration::int2d && di.dHat==2) ){
              if( ! haveMassMatrixforBEMTOOL ){
                cerr << " Two mass matrix for BEMTOOl in largs is not allowed " << endl;
                ffassert(0);
              }
              haveMassMatrixforBEMTOOL = true;
            }
      }
    } // atype FormBilinear
    else if(r == atype<const  BC_set  *>()){
      nbBC++;
    } // atype BC
  }

  if( nbBC >0 ){
    if( haveBemFormBilinear ){
      cerr << "Error: Dirichlet Boundary condition and BEM operator in the same variational form." << endl;
      ffassert(0);
    }
  }

  // mixed FEM-BEM terms
  if( haveBemFormBilinear && haveMassMatrixforBEMTOOL  && nbFB > 0 ){
    return 12;    // BEM + mass matrix ==> H-matrix, FEM
  }
  else if( haveBemFormBilinear && nbFB > 0){
    return 11;    // BEM+FEM
  }
  else if( haveBemFormBilinear && haveMassMatrixforBEMTOOL ){
      return 2;   // BEM + mass matrix ==> H-matrix
  }
  else if( haveBemFormBilinear ){
    return 1;    // BEM only
  }
  else{
    return 0;    // FEM only
  }
}

/**
       *  @brief  largs separate in two part :  BEM (H-matrix) and FEM 
       *  @param  largs list of argument of the Bilinear Form
       *  @param  largs_FEM list of argument for the FEM part
       *  @param  largs_BEM list of argument for the BEM part (included sometimes mass matrix ) that be compressed in H-matrix
       */

/*
  This function must be call if we have BEM Bilinear operator in a block.
  
*/
void separateFEMpartBemPart(const list<C_F0> & largs, list<C_F0> &largs_FEM, list<C_F0> &largs_BEM ){
  
  list<C_F0>::const_iterator ii, b_ib=largs.begin(), b_ie=largs.end(); 

  for( ii=b_ib;ii != b_ie;ii++){
    Expression e=ii->LeftValue();
    aType r = ii->left();
  
    // ***************************************
    // Case FormBillinear
    // ***************************************
    if (r==atype<const  FormBilinear *>() ){
      
      //
      const FormBilinear * bb=dynamic_cast<const  FormBilinear *>(e);
      const CDomainOfIntegration & di= *bb->di;
      // check the integration (keyword)

      BilinearOperator * Op=const_cast<  BilinearOperator *>(bb->b);
      if (Op == NULL) { if(mpirank == 0) cout << "dynamic_cast error" << endl; }

      size_t Opsize= Op->v.size();
      
      bool  haveBEMmass   = false;
      size_t indexBEMmass = 0;

      for(size_t jj=0; jj<Opsize; jj++){
        // attention la fonction test donne la ligne
        //  et la fonction test est en second
        BilinearOperator::K ll = Op->v[jj]; //  LinearComb<pair<MGauche,MDroit>,C_F0> BilinearOperator;
        pair<int,int> finc(ll.first.first), ftest(ll.first.second);

        // check if we have a mass matrix that we need to add to BEMTOOL 
        // cf bem.hpp: see pair<BemKernel*,double> getBemKernel(Stack stack, const list<C_F0> & largs).

        if( finc.first==0 && ftest.first==0)      // only first component for finc and ftest
          if( finc.second==0 && ftest.second==0 ) // only op_id=0
            if( (di.kind == CDomainOfIntegration::int1d && di.dHat==1) || (di.kind == CDomainOfIntegration::int2d && di.dHat==2) ){   
              
              haveBEMmass  = true;
              indexBEMmass  = jj;
            }
      }

      if( !haveBEMmass ){
        largs_FEM.push_back(*ii);
      }
      else{
        bool * partFEM = new bool[Opsize];
        for(size_t jj=0; jj<Opsize; jj++){
          partFEM[jj]= true;
        }
        partFEM[indexBEMmass] = false;

        // check 

        BilinearOperator * OpFEM = new BilinearOperator( *Op, partFEM );
        BilinearOperator * OpBEM = new BilinearOperator( Op->v[indexBEMmass].first, Op->v[indexBEMmass].second );

        largs_FEM.push_back( C_F0( new FormBilinear( &di, OpFEM ), r ) );
        largs_BEM.push_back( C_F0( new FormBilinear( &di, OpBEM ), r ) );

        delete [] partFEM;
      }
      // creation des deux listes

    } // atype FormBilinear
    else if(r == atype<const  BC_set  *>()){
      largs_FEM.push_back(*ii);
    } // atype BC
#ifndef FFLANG
#ifdef PARALLELE
    // ******************************************
    // Case BemKFormBilinear (KERNEL FORM ONLY)
    // ******************************************
    else if (r==atype<const BemFormBilinear *>() ){
      BemFormBilinear * bbtmp= dynamic_cast< BemFormBilinear *>(e);
      ffassert(bbtmp);
      int VVFBEM = bbtmp->type;

      if(VVFBEM ==1){
        BemKFormBilinear * bb= dynamic_cast< BemKFormBilinear *>(e);
        ffassert(bb);

        BemKFormBilinear * bbnew = new BemKFormBilinear( bb->di, FoperatorKBEM(bb->b->kbem, *(bb->b->fi), *(bb->b->ft) ) ); // marche ???
        largs_BEM.push_back( C_F0( bbnew, r ) ); 
      }else{
        cerr << "case VVFBEM noot coded yet. todo." << endl;
        ffassert(0);
      }  
      // On pourrait retourner l'element de largs. Mais on prefere en creer un nouveau pour etre coherent avec cree deleteNewLargs.
      // largs_BEM.push_back(*ii); 
    }
#endif
#endif
  }
}

/**
       *  @brief  Function to delete element of newlargs = list<C_F0> to avoid memory leak.
       *  @param  largs list of argument of the composite FESpace gene
       */

/* remark: only FormBilinear  and BemFormBilinear is deleted */
void deleteNewLargs(list<C_F0> &newlargs){
  list<C_F0>::iterator ii,ib=newlargs.begin(),ie=newlargs.end(); 
  for (ii=ib;ii != ie;ii++) {
    Expression e=ii->LeftValue();
    aType r = ii->left();
  
    // ***************************************
    // Case FormBillinear
    // ***************************************
    if (r==atype<const  FormBilinear *>() ){
      FormBilinear * bb=dynamic_cast< FormBilinear *>(e);
      //BilinearOperator * Op=dynamic_cast<  BilinearOperator *>(bb->b);
      for(size_t jj=0; jj < bb->b->v.size(); jj++ ){
        bb->b->v[jj].second.Destroy();
      }
      delete bb->b;
      delete bb;
    }
    // ****************************************************************
    // BC_set in newlargs correspond to the original BC_set in largs
    //   ==> implies not deleted this element.
    // ****************************************************************
    if (r==atype<const  BC_set *>() ){
      BC_set * bb=dynamic_cast< BC_set *>(e);
      /*
      //BilinearOperator * Op=dynamic_cast<  BilinearOperator *>(bb->b);
      for(size_t jj=0; jj < bb->b->v.size(); jj++ ){
        bb->b->v[jj].second.Destroy();
      }
      delete bb->b;
      */
      delete bb;
    }
#ifndef FFLANG
#ifdef PARALLELE
    // ******************************************
    // Case BemKFormBilinear (KERNEL FORM ONLY)
    // ******************************************
    else if (r==atype<const BemFormBilinear *>() ){
      BemFormBilinear * bbtmp= dynamic_cast< BemFormBilinear *>(e);
      ffassert(bbtmp);
      int VVFBEM = bbtmp->type;

      if(VVFBEM ==1){
        BemKFormBilinear * bb= dynamic_cast< BemKFormBilinear *>(e);
        ffassert(bb);
        FoperatorKBEM * b=const_cast<  FoperatorKBEM *>(bb->b);
        LOperaG * OpG = const_cast<LOperaG *>(b->fi);
        for(size_t jj=0; jj < OpG->v.size(); jj++ ){
          OpG->v[jj].second.Destroy();
        }
        LOperaD * OpD = const_cast<LOperaD *>(b->ft);
        for(size_t jj=0; jj < OpD->v.size(); jj++ ){
          OpD->v[jj].second.Destroy();
        }
        delete b->fi;
        delete b->ft;
        delete b;
        delete bb;
        //BemKFormBilinear * bbnew = new BemKFormBilinear( bb->di, FoperatorKBEM(bb->b->kbem, *(bb->b->fi), *(bb->b->ft) ) ); // marche ???
        //newlargs.push_back( C_F0( bbnew, r ) );
      }
      else{
        cerr << "coding only case VFBEM == 1. Other case todo." <<endl;
        ffassert(0);
      }
    }
#endif
#endif

    ii->Destroy();
  }
}


template<class R>
AnyType OpMatrixtoBilinearFormVG<R>::Op::operator()(Stack stack) const
{
  assert(b && b->nargs);

  pvectgenericfes  * pUh= GetAny<pvectgenericfes *>((*b->euh)(stack));
  pvectgenericfes  * pVh= GetAny<pvectgenericfes *>((*b->evh)(stack));
  ffassert( *pUh && *pVh ); 

  if( verbosity > 5){
    (*pUh)->printPointer();
    (*pVh)->printPointer();
  }
  int NpUh = (*pUh)->N; // number of fespace in pUh
  int NpVh = (*pVh)->N; // number of fespace in pVh

  KN<int> UhNbOfDf = (*pUh)->vectOfNbOfDF();
  KN<int> VhNbOfDf = (*pVh)->vectOfNbOfDF();

  KN<int> UhNbItem = (*pUh)->vectOfNbitem();
  KN<int> VhNbItem = (*pVh)->vectOfNbitem();

  KN<int> beginBlockUh(NpUh); // index of the first elment of a block
  KN<int> beginBlockVh(NpVh);

  // loop over the index 
  int UhtotalNbItem=0;
  if(verbosity>5) cout << "finc: FESpace" << endl;
  for(int i=0; i< NpUh; i++){
    if(verbosity>5) cout << "component i=" << i << ", NbItem["<<i<<"]=" <<  UhNbItem[i] << ", NbDof["<<i<<"]=" << UhNbOfDf[i] << endl;
    beginBlockUh[i] = UhtotalNbItem;
    UhtotalNbItem += UhNbItem[i];
    ffassert( UhNbItem[i] > 0 && UhNbOfDf[i] > 0);
  }

  int VhtotalNbItem=0;
  if(verbosity>5) cout << "ftest: FESpace" << endl;
  for(int i=0; i< NpVh; i++){
    if(verbosity>5) cout << "component i=" << i << ", NbItem["<<i<<"]=" <<  VhNbItem[i] << ", NbDof["<<i<<"]=" << VhNbOfDf[i] << endl;
    beginBlockVh[i] = VhtotalNbItem;
    VhtotalNbItem += VhNbItem[i];
    ffassert( VhNbItem[i] > 0 && VhNbOfDf[i] > 0);
  }
  
  // index for the construction of the block
  KN<int> indexBlockUh(UhtotalNbItem);
  KN<int> localIndexInTheBlockUh(UhtotalNbItem);
  { 
    // ========================
    //
    // varf([u0,u1,...,u4], ... ) 
    // varf([ [u0_blk1,u1_blk1],[u0_blk2,u1_blk2,u2_blk2] ], ... ) 

    // u4 correspond to u2_blk2 in the block varf
    // ============================================
    // For u4, on a :: current_index = 4
    //              :: indexBlockUh = 2
    //              :: localIndexInThBlock = 3
    int current_index=0;
    for(int i=0; i<NpUh; i++){
      for(int j=0; j<UhNbItem[i]; j++){
        indexBlockUh[current_index] = i;
        localIndexInTheBlockUh[current_index] = j;
        current_index++;
      }
    }
    ffassert(current_index==UhtotalNbItem);
  }

  KN<int> indexBlockVh(VhtotalNbItem);
  KN<int> localIndexInTheBlockVh(VhtotalNbItem);
  { 
    int current_index=0;
    for(int i=0; i<NpVh; i++){
      for(int j=0; j<VhNbItem[i]; j++){
        indexBlockVh[current_index] = i;
        localIndexInTheBlockVh[current_index] = j;
        current_index++;
      }
    }
    ffassert(current_index==VhtotalNbItem);
  }
  cout <<"========================================================" << endl;
  cout <<"=                                                      =" << endl;
  cout <<"= indexBlockUh=                                        =" << endl;
  cout << indexBlockUh << endl;
  cout <<"=                                                      =" << endl;
  cout <<"= localIndexInTheBlockUh=                              =" << endl;
  cout << localIndexInTheBlockUh << endl;

  cout <<"========================================================" << endl;
  cout <<"=                                                      =" << endl;
  cout <<"= indexBlockVh=                                        =" << endl;
  cout << indexBlockVh << endl;
  cout <<"=                                                      =" << endl;
  cout <<"= localIndexInTheBlockVh=                              =" << endl;
  cout << localIndexInTheBlockVh << endl;


  #ifndef FFLANG
  #ifdef PARALLELE
  cout << "====    define parallele  =====" << endl;
  #endif
  #endif
  
  //
  const list<C_F0> & largs=b->largs; 

  // creation du nouvelle liste de parametre de la liste des arguments:
  //list<C_F0>  newlargs = creationBilinearForm( largs, localIndexInTheBlockUh, localIndexInTheBlockVh );  

  
  cout << "=== out  largs ===" << endl;
  cout <<"=                                                      =" << endl;
  listOfComponentBilinearForm(largs);
  cout <<"=                                                      =" << endl;
  /*
  cout << "=== out  new largs ===" << endl;
  cout <<"=                                                      =" << endl;
  listOfComponentBilinearForm(newlargs);
  cout <<"=                                                      =" << endl;
  */

  list<C_F0>  newlargs = creationLargsForCompositeFESpace( largs, NpUh, NpVh, indexBlockUh, indexBlockVh ); 
  
  cout << "=== out  largs ===" << endl;
  cout <<"=                                                      =" << endl;
  listOfComponentBilinearForm(newlargs);
  cout <<"=                                                      =" << endl;
  

  KNM< list<C_F0> > block_largs = computeBlockLargs( newlargs, NpUh, NpVh, indexBlockUh, indexBlockVh );
  
  if(verbosity > 9){
    // check the list of <<largs>> for each block
    cout <<"========================================================" << endl;
    cout <<"=                                                      =" << endl;
    cout <<"= check the list of largs of each block                =" << endl;
    cout <<"=                                                      =" << endl;
    for( int i=0; i<NpUh; i++){
      for( int j=0; j<NpVh; j++){
        cout<< " block ( "<< i << " , " << j <<  " ) " ;
        const list<C_F0> & b_largs=block_largs(i,j); 
        cout<< ", size of the list=" << b_largs.size() << endl;
        // impression des information de la composition largs
        list<C_F0>::const_iterator b_ii,b_ib=b_largs.begin(),b_ie=b_largs.end(); 
        for (b_ii=b_ib;b_ii != b_ie;b_ii++){
          Expression e=b_ii->LeftValue();
          aType r = b_ii->left();
          cout << "e=" << e << ", r=" << r << endl;
        }
      }
    }
  }
  
  // Info necessaire :: " block_largs, localIndexInTheBlockUh, localIndexInTheBlockVh, NpUh, NpVh  
  changeComponentFormCompositeFESpace( localIndexInTheBlockUh, localIndexInTheBlockVh, block_largs );

  if(verbosity > 9){
    cout <<"========================================================" << endl;
    cout <<"=                                                      =" << endl;
    cout <<"= check the component of Bilinear Form                 =" << endl;
    cout <<"=                                                      =" << endl;
    listOfComponentBilinearForm(newlargs);
    cout <<"=                                                      =" << endl;
    cout <<"========================================================" << endl;
  }
  //===   Information of the global matrix    ===// 

  // check if we have a square matrix
  bool A_is_square= (void*)pUh == (void*)pVh || ((*pUh)->totalNbOfDF()) == ( (*pVh)->totalNbOfDF()) ;
  cout << "A_is_square=" << A_is_square << endl;

  // === simple check if A is symetrical === // 
  // voir avec les autres.
  bool A_is_maybe_sym = (void*)pUh == (void*)pVh; 

  // VF == true => VF type of Matrix
  bool VF=isVF(b->largs);    //=== used to set the solver ??? block matrix ??? ===/

  // set parameteer of the matrix :: 
  Data_Sparse_Solver ds;
  ds.factorize=0;
  ds.initmat=true;
  int np = OpCall_FormBilinear_np::n_name_param - NB_NAME_PARM_HMAT;
  SetEnd_Data_Sparse_Solver<R>(stack,ds, b->nargs,np);

  // set ds.sym = 0 
  ds.sym = 0;
  if(verbosity)
    cout << " we consider the block matrix as a non symetric matrix " << endl; 

  // J'ai repris ce qu'il y avait. 
  // PAC(e)     :: Attention peut être pas compatible avec les matrices bloques.
  // A repenser :: surtout pour le parametre symetrique? on le met ce parametre à zéro pour l'instant.
  // set ds.sym = 0 

  ds.sym = 0;
  if(verbosity)
    cout << " === we consider the block matrix as a non symetric matrix === (to be change in the future)" << endl; 

  if (! A_is_square )
   {
     if(verbosity>3) cout << " -- the solver  is un set  on rectangular matrix  " << endl;
    }

  // A quoi cela correspond?? Gestion du stack + autre
  WhereStackOfPtr2Free(stack)=new StackOfPtr2Free(stack);// FH aout 2007

  Matrice_Creuse<R> & A( * GetAny<Matrice_Creuse<R>*>((*a)(stack)));
  if(init) A.init(); //
  if(verbosity)                                           
    cout << " A.N=" <<  A.N() << endl;
    cout << " A.M=" <<  A.M() << endl;
  if( ! pUh || ! pVh) return SetAny<Matrice_Creuse<R>  *>(&A);  //

  // need to define the size of the entire matrix here ==> execution error
  A.resize( (*pVh)->totalNbOfDF(), (*pUh)->totalNbOfDF() ); 
  // test function (Vh) are the line
  // inconnu function (Uh) are the column

  // Assemble the variationnal form
  int maxJVh=NpVh;
  
  int offsetMatrixUh = 0;
  // loop over the block
  for( int i=0; i<NpUh; i++){
    int offsetMatrixVh = 0;
    if( ds.sym > 0 ){ maxJVh=(i+1); ffassert(maxJVh<NpVh);}
    for( int j=0; j<maxJVh; j++){
      cout << "offsetMatrixUh= " << offsetMatrixUh << ", offsetMatrixVh= " << offsetMatrixVh << endl;
      
      // construction du block (i,j)
      const list<C_F0> & b_largs=block_largs(i,j); 

      //const void * PUh = (void *) (*pUh)->vect[i]->getpVh();
      //const void * PVh = (void *) (*pVh)->vect[j]->getpVh();

      // size of the block
      int N_block = UhNbOfDf[i];
      int M_block = VhNbOfDf[j];
      /*
      Matrice_Creuse<R> *CCC = new Matrice_Creuse<R>() ;
      CCC->resize(M_block,N_block); // test function (Vh) are the line and inconnu function (Uh) are the column
      cout << "block:  i=" << i << "j=" << j <<  " (N,M)=" << M_block << " " << N_block << endl;
      */
      list<C_F0>::const_iterator b_largs_ii,b_ib=b_largs.begin(),b_ie=b_largs.end(); 
      for (b_largs_ii=b_ib;b_largs_ii != b_ie;b_largs_ii++){
        
        Matrice_Creuse<R> *CCC = new Matrice_Creuse<R>() ;
        CCC->resize(M_block,N_block); // test function (Vh) are the line and inconnu function (Uh) are the column
        cout << "block:  i=" << i << "j=" << j <<  " (N,M)=" << M_block << " " << N_block << endl;
        
#ifndef FFLANG
#ifdef PARALLELE
        //list<C_F0>::const_iterator b_largs_ii,b_ib=b_largs->begin(),b_ie=b_largs->end(); 
        //for (b_largs_ii=b_ib;b_largs_ii != b_ie;b_largs_ii++){
        if (b_largs_ii->left() == atype<const BemFormBilinear *>() ){
          list<C_F0> b_largs_tmp;
          b_largs_tmp.push_back(*b_largs_ii);
          const list<C_F0> & b_largs_zz = b_largs_tmp;
          
          int VFBEM = typeVFBEM(b_largs_zz,stack);
          if(VFBEM == 2){ cerr << " not implemented with BEM POTENTIAL" << endl; ffassert(0);}
          Data_Bem_Solver dsbem;
          dsbem.factorize=0;
          dsbem.initmat=true;
          SetEnd_Data_Bem_Solver<R>(stack, dsbem, b->nargs,OpCall_FormBilinear_np::n_name_param);  // LIST_NAME_PARM_HMAT

          HMatrixVirt<R> ** Hmat = new HMatrixVirt<R> *();
          /*
          bool samemesh = (void*) (*pUh)->vect[i]->getppTh() == (void*) (*pVh)->vect[j]->getppTh();  // same Fem2D::Mesh     +++ pot or kernel
          if (VFBEM==1)
            ffassert (samemesh);
          if(init)
            *Hmat =0;
          *Hmat =0;
          if( *Hmat)
            delete *Hmat;
          *Hmat =0;
          */

          //
          // avoir dans le futur si la difference entre bloc diagonal et bloc non diagonal a un sens.
          //
          if( i==j ){

            bool samemesh = (void*) (*pUh)->vect[i]->getppTh() == (void*) (*pVh)->vect[j]->getppTh();  // same Fem2D::Mesh     +++ pot or kernel
            if (VFBEM==1)
              ffassert (samemesh);
            if(init)
              *Hmat =0;
            *Hmat =0;
            if( *Hmat)
              delete *Hmat;
            *Hmat =0;


            // block diagonal matrix
            if( (*pUh)->typeFE[i] == 4 && (*pVh)->typeFE[j] == 4 ){
              ffassert( i==j ); // If not a block diagonal not coded yet
              // MeshS --- MeshS
              // ==== FESpace 3d Surf: inconnue et test ===
              const FESpaceS * PUh = (FESpaceS *) (*pUh)->vect[i]->getpVh();
              const FESpaceS * PVh = (FESpaceS *) (*pVh)->vect[j]->getpVh();

              creationHMatrixtoBEMForm<R, MeshS, FESpaceS, FESpaceS>(PUh, PVh, VFBEM, 
                              b_largs_zz, stack, dsbem, Hmat);

            }
            else if( (*pUh)->typeFE[i] == 5 && (*pVh)->typeFE[j] == 5 ){
              ffassert( i==j ); // If not a block diagonal not coded yet
              // MeshL --- MeshL
              // ==== FESpace 3d Curve: inconnue et test ===
              const FESpaceL * PUh = (FESpaceL *) (*pUh)->vect[i]->getpVh();
              const FESpaceL * PVh = (FESpaceL *) (*pVh)->vect[j]->getpVh();

              creationHMatrixtoBEMForm<R, MeshL, FESpaceL, FESpaceL> ( PUh, PVh, VFBEM, 
                              b_largs_zz, stack, dsbem, Hmat );
            }
            else{
              cerr << " BEM bilinear form " << endl;
              cerr << " Block ("<< i <<" ,"<< j << ")" << endl;
              cerr << " =: Pas prise en compte des FESpace inconnue de type := "<< typeFEtoString( (*pUh)->typeFE[i] ) << endl;
              cerr << " =:                 avec des FESpace test de type    := "<< typeFEtoString( (*pVh)->typeFE[j] ) << endl;
              ffassert(0);
            }

            // creation de la matrice dense 
            KNM<R>* M= HMatrixVirtToDense< KNM<R>, R >(Hmat);

            HashMatrix<int,R> *phm= new HashMatrix<int,R>(*M);
            MatriceCreuse<R> *pmc(phm);

            Matrice_Creuse<R> BBB;
            BBB.A=0;
            BBB.A.master(pmc);

            A.pHM()->Add( BBB.pHM(), R(1), false, offsetMatrixVh, offsetMatrixUh ); // test function (Vh) are the line and inconnu function (Uh) are the column

            M->destroy();
            delete M;
            BBB.destroy();
          }
          else{
            
            bool samemesh = (void*) (*pUh)->vect[i]->getppTh() == (void*) (*pVh)->vect[j]->getppTh();  // same Fem2D::Mesh     +++ pot or kernel
          
            if(init)
              *Hmat =0;
            //*Hmat =0;
            if( *Hmat)
              delete *Hmat;
            *Hmat =0;
            
            
            // block non diagonal matrix        
            if( (*pUh)->typeFE[i] == 5 && (*pVh)->typeFE[j] == 2 ){
              // case Uh[i] == MeshL et Vh[j] = Mesh2  // Est ce que cela a un sens?
              
              cout << " === creation de la matrice BEM pour un bloc non diagonaux === " << endl;
              //ffassert(0);
              const FESpaceL * PUh = (FESpaceL *) (*pUh)->vect[i]->getpVh();
              creationHMatrixtoBEMForm<R, MeshL, FESpaceL, FESpaceL> ( PUh, PUh, VFBEM, 
                          b_largs_zz, stack, dsbem, Hmat );

            }
            /*
            else if( (*pUh)->typeFE[i] == 2 && (*pVh)->typeFE[j] == 5 ){
              // case Uh[i] == Mesh2 et Vh[j] = MeshL
              //
              cerr << " BEM bilinear form " << endl;
              cerr << " Pour un bloc non diagonal, on ne prend pas en compte: " << endl;
              cerr << " =: Pas prise en compte des FESpace inconnue de type := "<< typeFEtoString( (*pUh)->typeFE[i] ) << endl;
              cerr << " =:                 avec des FESpace test de type    := "<< typeFEtoString( (*pVh)->typeFE[j] ) << endl; 
              cerr << "not coded yet=" << endl;
              ffassert(0);
              
            }
            else if( (*pUh)->typeFE[i] == 4 && (*pVh)->typeFE[j] == 4 ){
              cerr << " BEM bilinear form " << endl;
              cerr << " Pour un bloc non diagonal, on ne prend pas en compte: " << endl;
              cerr << " =: Pas prise en compte des FESpace inconnue de type := "<< typeFEtoString( (*pUh)->typeFE[i] ) << endl;
              cerr << " =:                 avec des FESpace test de type    := "<< typeFEtoString( (*pVh)->typeFE[j] ) << endl;
              ffassert(0);
            }
            else if( (*pUh)->typeFE[i] == 5 && (*pVh)->typeFE[j] == 5 ){
              cerr << " BEM bilinear form " << endl;
              cerr << " Pour un bloc non diagonal, on ne prend pas en compte: " << endl;
              cerr << " =: Pas prise en compte des FESpace inconnue de type := "<< typeFEtoString( (*pUh)->typeFE[i] ) << endl;
              cerr << " =:                 avec des FESpace test de type    := "<< typeFEtoString( (*pVh)->typeFE[j] ) << endl;
              ffassert(0);
            }
            */   
            else{
              cerr << " BEM bilinear form " << endl;
              cerr << " Block ("<< i <<" ,"<< j << ")" << endl;
              cerr << " =: Pas prise en compte des FESpace inconnue de type := "<< typeFEtoString( (*pUh)->typeFE[i] ) << endl;
              cerr << " =:                 avec des FESpace test de type    := "<< typeFEtoString( (*pVh)->typeFE[j] ) << endl;
              ffassert(0);
            }
            
            // creation de la matrice dense 
            
            KNM<R>* M = HMatrixVirtToDense< KNM<R>, R >(Hmat);
            
            HashMatrix<int,R> *phm= new HashMatrix<int,R>(*M);
            MatriceCreuse<R> *pmc(phm);
            
            Matrice_Creuse<R> *BBB=new Matrice_Creuse<R>();
            BBB->A=0;
            BBB->A.master(pmc);
            //BBB->resize(356,356);

            // BEM matrix is constructed with different FESpace
            ffassert( (*pUh)->vect[i]->getpVh() != (*pVh)->vect[j]->getpVh() ) ;
            
            
            if( (*pUh)->typeFE[i] == 5 && (*pVh)->typeFE[j] == 2 ){
              // case Uh[i] == MeshL et Vh[j] = Mesh2 
              const FESpaceL * PUh = (FESpaceL *) (*pUh)->vect[i]->getpVh();
              const FESpace * PVh = (FESpace *) (*pVh)->vect[j]->getpVh();
              // construction of the matrix of interpolation
              
              //Matrice_Creuse<double> *  MI_BBB=new Matrice_Creuse<double>(); // = buildMatrixInterpolationForCompositeFESpace<double,FESpaceL,FESpace>( PUh, PVh  );
              Matrice_Creuse<double> *  MI_BBB = buildMatrixInterpolationForCompositeFESpace<double,FESpaceL,FESpace>( PUh, PVh  );

              //MI_BBB->resize(356,2922);
              // multiplication matrix*matrix
            
              MatriceMorse<double> *mA= MI_BBB->pHM();
              MatriceMorse<R> *mB= BBB->pHM();            

              
              cout << "A=MI " << MI_BBB->N() << " " << MI_BBB->M() << endl;
              cout << "B=BBB " << BBB->N() << " " << BBB->M() << endl;

              ffassert( MI_BBB->M() >= BBB->M() ); 
              
              ffassert( MI_BBB->N() == BBB->M() );
              
              MatriceMorse<R> *mAB=new MatriceMorse<R>(MI_BBB->M(), BBB->M(),0,0);
              AddMul<int,double,R,R>(*mAB,*mA,*mB,true,false); // BBB=MI_BBB'*BBB;
              
              A.pHM()->Add( mAB, R(1), false, offsetMatrixVh, offsetMatrixUh ); // test function (Vh) are the line and inconnu function (Uh) are the column
              
              delete mAB;
              MI_BBB->destroy();
              delete MI_BBB;
              
            }
            else{
              cerr << "==== to do ==== " << endl;
              ffassert(0);
            }
            M->destroy();
            delete M;
            BBB->destroy();
            delete BBB;
            
          }
          if( *Hmat)
            delete *Hmat;
          delete Hmat;

        }
        else{
#endif
#endif
          // case BC_set or BilinearForm 
          Matrice_Creuse<R> &BBB(*CCC);

          list<C_F0> b_largs_tmp;
          b_largs_tmp.push_back(*b_largs_ii);
          const list<C_F0> & b_largs_zz = b_largs_tmp;
          // cas ::  Mesh, v_fes, v_fes
          if( (*pUh)->typeFE[i] == 2 && (*pVh)->typeFE[j] == 2 ){

            // ==== FESpace 2d : inconnue et test  ===
            const FESpace * PUh = (FESpace *) (*pUh)->vect[i]->getpVh();
            const FESpace * PVh = (FESpace *) (*pVh)->vect[j]->getpVh();
            creationBlockOfMatrixToBilinearForm< R, Mesh, FESpace,FESpace>( PUh, PVh, ds.sym, ds.tgv, b_largs_zz, stack, BBB);
          }
          // cas ::  Mesh3, v_fes3, v_fes3 
          else if( (*pUh)->typeFE[i] == 3 && (*pVh)->typeFE[j] == 3 ){

            // ==== FESpace 3d : inconnue et test ===
            const FESpace3 * PUh = (FESpace3 *) (*pUh)->vect[i]->getpVh();
            const FESpace3 * PVh = (FESpace3 *) (*pVh)->vect[j]->getpVh();
            creationBlockOfMatrixToBilinearForm<R,Mesh3,FESpace3,FESpace3>( PUh, PVh, ds.sym, ds.tgv, b_largs_zz, stack, BBB);
          }
          // cas :: MeshS, v_fesS, v_fesS 
          else if( (*pUh)->typeFE[i] == 4 && (*pVh)->typeFE[j] == 4 ){

            // ==== FESpace 3d Surf: inconnue et test ===
            const FESpaceS * PUh = (FESpaceS *) (*pUh)->vect[i]->getpVh();
            const FESpaceS * PVh = (FESpaceS *) (*pVh)->vect[j]->getpVh();
            creationBlockOfMatrixToBilinearForm<R,MeshS,FESpaceS,FESpaceS>( PUh, PVh, ds.sym, ds.tgv, b_largs_zz, stack, BBB);
          }
          // cas :: MeshL, v_fesL, v_fesL
          else if( (*pUh)->typeFE[i] == 5 && (*pVh)->typeFE[j] == 5 ){

            // ==== FESpace 3d Curve: inconnue et test ===
            const FESpaceL * PUh = (FESpaceL *) (*pUh)->vect[i]->getpVh();
            const FESpaceL * PVh = (FESpaceL *) (*pVh)->vect[j]->getpVh();
            creationBlockOfMatrixToBilinearForm<R,MeshL,FESpaceL,FESpaceL>( PUh, PVh, ds.sym, ds.tgv, b_largs_zz, stack, BBB);
          }
          // cas :: MeshL, v_fesL, v_fes
          else if( (*pUh)->typeFE[i] == 5 && (*pVh)->typeFE[j] == 2 ){

            // ==== FESpace 3d Curve: inconnue et 2d : test ===
            const FESpaceL * PUh = (FESpaceL *) (*pUh)->vect[i]->getpVh();
            const FESpace * PVh = (FESpace *) (*pVh)->vect[j]->getpVh();
            creationBlockOfMatrixToBilinearForm<R,MeshL,FESpaceL,FESpace>( PUh, PVh, ds.sym, ds.tgv, b_largs_zz, stack, BBB);
          }
          // cas :: MeshL, v_fes, v_fesL
          else if( (*pUh)->typeFE[i] == 2 && (*pVh)->typeFE[j] == 5 ){

            // ==== FESpace 2d: inconnue et 3d Curve: test ===
            const FESpace * PUh = (FESpace *) (*pUh)->vect[i]->getpVh();
            const FESpaceL * PVh = (FESpaceL *) (*pVh)->vect[j]->getpVh();
            creationBlockOfMatrixToBilinearForm<R,MeshL,FESpace,FESpaceL>( PUh, PVh, ds.sym, ds.tgv, b_largs_zz, stack, BBB);
          }
          // cas :: new OpMatrixtoBilinearForm< double, MeshS, v_fesS, v_fes3 >,      // 3D Surf / 3D volume on meshS
          else if( (*pUh)->typeFE[i] == 4 && (*pVh)->typeFE[j] == 3 ){

            // ==== FESpace 3d Surf: inconnue et 3d : test ===
            const FESpaceS * PUh = (FESpaceS *) (*pUh)->vect[i]->getpVh();
            const FESpace3 * PVh = (FESpace3 *) (*pVh)->vect[j]->getpVh();
            creationBlockOfMatrixToBilinearForm<R,MeshS,FESpaceS,FESpace3>( PUh, PVh, ds.sym, ds.tgv, b_largs_zz, stack, BBB);
          } 
          // cas :: new OpMatrixtoBilinearForm< double, MeshS, v_fes3, v_fesS >,     // 3D volume / 3D Surf on meshS
          else if( (*pUh)->typeFE[i] == 3 && (*pVh)->typeFE[j] == 4 ){

            // ==== FESpace 3d : inconnue et 3d Surf : test ===
            const FESpace3 * PUh = (FESpace3 *) (*pUh)->vect[i]->getpVh();
            const FESpaceS * PVh = (FESpaceS *) (*pVh)->vect[j]->getpVh();
            creationBlockOfMatrixToBilinearForm<R,MeshS,FESpace3,FESpaceS>( PUh, PVh, ds.sym, ds.tgv, b_largs_zz, stack, BBB);
          } 
          // cas :: new OpMatrixtoBilinearForm< double, MeshL, v_fesL, v_fesS >,       // 3D curve / 3D Surf on meshL
          else if( (*pUh)->typeFE[i] == 5 && (*pVh)->typeFE[j] == 4 ){

            // ====  FESpace 3d Curve : inconnue et 3d Surf : test ===
            const FESpaceL * PUh = (FESpaceL *) (*pUh)->vect[i]->getpVh();
            const FESpaceS * PVh = (FESpaceS *) (*pVh)->vect[j]->getpVh();
            creationBlockOfMatrixToBilinearForm<R,MeshL,FESpaceL,FESpaceS>( PUh, PVh, ds.sym, ds.tgv, b_largs_zz, stack, BBB);
          }
          // cas :: new OpMatrixtoBilinearForm< double, MeshL, v_fesS, v_fesL >);       // 3D Surf / 3D curve on meshL
          else if( (*pUh)->typeFE[i] == 4 && (*pVh)->typeFE[j] == 5 ){

            // ====  FESpace 3d Surf : inconnue et 3d Curve : test ===
            const FESpaceS * PUh = (FESpaceS *) (*pUh)->vect[i]->getpVh();
            const FESpaceL * PVh = (FESpaceL *) (*pVh)->vect[j]->getpVh();
            creationBlockOfMatrixToBilinearForm<R,MeshL,FESpaceS,FESpaceL>( PUh, PVh, ds.sym, ds.tgv, b_largs_zz, stack, BBB);
          }
          else{
            cerr << " =: Pas prise en compte des FESpace inconnue de type := "<< typeFEtoString( (*pUh)->typeFE[i] ) << endl;
            cerr << " =:                 avec des FESpace test de type    := "<< typeFEtoString( (*pVh)->typeFE[j] ) << endl;
            ffassert(0);
          }

          A.pHM()->Add( BBB.pHM(), R(1), false, offsetMatrixVh, offsetMatrixUh ); // test function (Vh) are the line and inconnu function (Uh) are the column
#ifndef FFLANG
#ifdef PARALLELE
        }
#endif
#endif
      //cout << "BBB=" << BBB<< endl;
      //??? // A.pHM()->Add( BBB.pHM(), R(1), false, offsetMatrixVh, offsetMatrixUh ); // test function (Vh) are the line and inconnu function (Uh) are the column
      //cout << "BBB=" << BBB<< endl;
      //cout << "A=" << A << endl;
      delete CCC;

      } // end loop bb_largs_ii
    offsetMatrixVh += VhNbOfDf[j];
    } // end loop j
    offsetMatrixUh += UhNbOfDf[i];
  } // end loop i
  
  A.pHM()->half = ds.sym;
  if (A_is_square)
    SetSolver(stack,VF,*A.A,ds);

  // === re-szet the original value of number of the component in the 
  //          - BilinearForm 
  //          - BC_set. 
  // ===

  // need to reverse component because we don't know how to delete properly BC_set
  reverseChangeComponentFormCompositeFESpace( beginBlockUh, beginBlockVh, 
          block_largs);
  
  

  cout <<"========================================================" << endl;
  cout <<"=                                                      =" << endl;
  cout <<"= check the component of Bilinear Form  (Final)        =" << endl;
  cout <<"=                                                      =" << endl;
  listOfComponentBilinearForm(largs);
  cout <<"=                                                      =" << endl;
  cout <<"========================================================" << endl;

  cout <<"========================================================" << endl;
  cout <<"=                                                      =" << endl;
  cout <<"= check the component of Bilinear Form  (newlargs Final)        =" << endl;
  cout <<"=                                                      =" << endl;
  listOfComponentBilinearForm(newlargs);
  cout <<"=                                                      =" << endl;
  cout <<"========================================================" << endl;



  deleteNewLargs(newlargs);
  newlargs.clear();
  return SetAny<Matrice_Creuse<R>  *>(&A);
}

#endif