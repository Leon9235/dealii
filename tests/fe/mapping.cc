// $Id$
// Copyright (C) 2001, 2002, 2003 Ralf Hartmann
//
// Shows the shape functions implemented and computes the area of cells.

#include <base/quadrature_lib.h>
#include <base/logstream.h>
#include <lac/vector.h>
#include <grid/tria.h>
#include <grid/tria_iterator.h>
#include <dofs/dof_accessor.h>
#include <grid/grid_generator.h>
#include <grid/grid_out.h>
#include <grid/tria_boundary_lib.h>
#include <fe/mapping_cartesian.h>
#include <fe/mapping_q1.h>
#include <fe/mapping_q.h>
#include <fe/fe_q.h>
#include <fe/fe_values.h>
#include <fe/fe.h>
#include <vector>
#include <fstream>
#include <string>

#ifdef HAVE_STD_STRINGSTREAM
#  include <sstream>
#  define SSTREAM std::ostringstream
#  define ENDSTRING
#else
#  include <strstream>
#  define SSTREAM std::ostrstream
#  define ENDSTRING << std::ends
#endif

#define PRECISION 2

char fname[50];

template<int dim>
inline void
plot_transformation(Mapping<dim> &mapping,
		    FiniteElement<dim> &fe,
		    typename DoFHandler<dim>::cell_iterator &cell,
		    const std::string& name)
{
  const unsigned int div = 7;

  QTrapez<1> q_trapez;
  QIterated<dim> q(q_trapez, div);
  FEValues<dim> fe_values(mapping, fe, q,
			  UpdateFlags(update_q_points
				      | update_JxW_values));

  fe_values.reinit(cell);

  deallog.push(name);
  
  unsigned int k=0;
  for (unsigned int nz=0; nz<=((dim>2) ? div : 0); ++nz)
    {
      for (unsigned int ny=0; ny<=((dim>1) ? div : 0); ++ny)
	{
	  for (unsigned int nx=0; nx<=div; ++nx)
	    {
	      deallog << fe_values.quadrature_point(k);
	      double J = fe_values.JxW(k) / q.weight(k);
	      deallog << ' ' << J << std::endl;
	      ++k;
	    }
	  deallog << std::endl;
	}
      deallog << std::endl;  
    }
  deallog.pop();
}



template<int dim>
inline void
plot_faces(Mapping<dim> &mapping,
	   FiniteElement<dim> &fe,
	   typename DoFHandler<dim>::cell_iterator &cell,
	   const std::string& name)
{
  deallog.push(name);

  QGauss4<dim-1> q;
  const unsigned int nq = (unsigned int) (.01 + pow(q.n_quadrature_points, 1./(dim-1)));

  FEFaceValues<dim> fe_values(mapping, fe, q,
			      UpdateFlags(update_q_points
					  | update_JxW_values
					  | update_normal_vectors));

  for (unsigned int face_nr=0;
       face_nr < GeometryInfo<dim>::faces_per_cell;
       ++ face_nr)
    {
      fe_values.reinit(cell, face_nr);

      unsigned int k=0;
      for (unsigned int ny=0; ny<((dim>2) ? nq : 1); ++ny)
	{
	  for (unsigned int nx=0; nx<nq; ++nx)
	    {
	      const Point<dim> x = fe_values.quadrature_point(k);
	      const Point<dim>& n = fe_values.normal_vector(k);
	      const double ds = fe_values.JxW(k);
	      
	      deallog << x << '\t' << n << '\t' << ds << std::endl;
	      ++k;
	    }
	  deallog << std::endl;
	}
      deallog << std::endl;      
    }
  deallog.pop();
}



template<int dim>
inline void
plot_subfaces(Mapping<dim> &mapping,
	      FiniteElement<dim> &fe,
	      typename DoFHandler<dim>::cell_iterator &cell,
	      const std::string& name)
{
  deallog.push(name);

  QGauss4<dim-1> q;
  const unsigned int nq = (unsigned int) (.01 + pow(q.n_quadrature_points, 1./(dim-1)));

  FESubfaceValues<dim> fe_values(mapping, fe, q,
				 UpdateFlags(update_q_points
					     | update_normal_vectors));
  for (unsigned int face_nr=0;
       face_nr < GeometryInfo<dim>::faces_per_cell;
       ++ face_nr)
    for (unsigned int sub_nr=0;
	 sub_nr < GeometryInfo<dim>::subfaces_per_face;
	 ++ sub_nr)
      {
	fe_values.reinit(cell, face_nr, sub_nr);
	
	const std::vector<Point<dim> > &normals
	  =fe_values.get_normal_vectors();
	
	unsigned int k=0;
	for (unsigned int ny=0; ny<((dim>2) ? nq : 1); ++ny)
	  {
	    for (unsigned int nx=0; nx<nq; ++nx)
	      {
		Point<dim> x = fe_values.quadrature_point(k);
		Tensor<1,dim> n = normals[k];
		deallog << x << '\t' << n << std::endl;
		++k;
	      }
	    deallog << std::endl;
	  }
	deallog << std::endl;
      }
  deallog.pop();
}



template<>
inline void
plot_faces(Mapping<1>&,
	   FiniteElement<1>&,
	   DoFHandler<1>::cell_iterator&,
	   const std::string&)
{}



template<>
inline void
plot_subfaces(Mapping<1>&,
	      FiniteElement<1>&,
	      DoFHandler<1>::cell_iterator&,
	      const std::string&)
{}



template<int dim>
inline void
compute_area(Mapping<dim> &mapping,
	     FiniteElement<dim> &fe,
	     typename DoFHandler<dim>::cell_iterator &cell)
{
  QGauss4<dim> gauss4;
  FEValues<dim> fe_values(mapping, fe, gauss4,
			UpdateFlags(update_JxW_values));
  fe_values.reinit(cell);
  const std::vector<double> &JxW=fe_values.get_JxW_values();

  double area=0;
  for (unsigned int i=0; i<fe_values.n_quadrature_points; ++i)
    area+=JxW[i];
  deallog << "  area=" << area << std::endl;
}


template<int dim>
void create_triangulations(std::vector<Triangulation<dim> *> &,
			   std::vector<Boundary<dim> *> &,
			   std::vector<double> &)
{
  Assert(false, ExcNotImplemented());
}



std::vector<std::vector<unsigned int> > show;
unsigned int mapping_size;


template<>
void create_triangulations(std::vector<Triangulation<1> *> &tria_ptr,
			   std::vector<Boundary<1> *> &,
			   std::vector<double> &exact_areas)
{
  show.resize(1, std::vector<unsigned int> (mapping_size,0));
  Triangulation<1> *tria=new Triangulation<1>();
  tria_ptr.push_back(tria);
  GridGenerator::hyper_cube(*tria, 1., 3.);
  exact_areas.push_back(2.);
  show[0][0]=1;
  show[0][1]=1;
  show[0][5]=1;
}



template<>
void create_triangulations(std::vector<Triangulation<2> *> &tria_ptr,
			   std::vector<Boundary<2> *> &boundary_ptr,
			   std::vector<double> &exact_areas)
{
  Triangulation<2> *tria;
  show.clear();
  show.resize(4, std::vector<unsigned int> (mapping_size,0));
				   // tria0: 3x3 square rotated
  if (1)
    {
      tria=new Triangulation<2>();
      tria_ptr.push_back(tria);
      const double left = 1.;
      const double right = 4.;
      
      const Point<2> vertices[4] = { Point<2>(left,left),
				       Point<2>(right,left),
				       Point<2>(right,right),
				       Point<2>(left,right)  };
      const int cell_vertices[1][4] = { { 1,2,3,0 } };
      std::vector<CellData<2> > cells (1, CellData<2>());
      for (unsigned int j=0; j<4; ++j)
	cells[0].vertices[j] = cell_vertices[0][j];
      cells[0].material_id = 0;
      
      tria->create_triangulation (std::vector<Point<2> >(&vertices[0], &vertices[4]),
				 cells,
				 SubCellData());
      exact_areas.push_back(9.);
    }
  
				   // tria1: arbitrary quadrilateral
  if (1)
    {
      tria=new Triangulation<2>();
      tria_ptr.push_back(tria);
      GridGenerator::hyper_cube(*tria, 1., 3.);
      Point<2> &v0=tria->begin_quad()->vertex(0);
      v0(0) = 0.;
      Point<2> &v2=tria->begin_quad()->vertex(2);
      v2(0) = 5.;
      v2(1) = 4.;
      exact_areas.push_back(7.);
      show[1][0] = 1;
    }
  
				   // tria2: crazy cell
  if (2)
    {
      Boundary<2> *boundary1=new HyperBallBoundary<2>(Point<2>(3,1), 2);
      Boundary<2> *boundary2=new HyperBallBoundary<2>(Point<2>(2,5), sqrt(5));
      boundary_ptr.push_back(boundary1);
      boundary_ptr.push_back(boundary2);      
      tria=new Triangulation<2>();
      tria_ptr.push_back(tria);
      GridGenerator::hyper_cube(*tria, 1., 5.);
      Point<2> &v2 = tria->begin_active()->vertex(2);
      Point<2> &v3 = tria->begin_active()->vertex(3);
      v2(0) = 3.;
      v2(1) = 3.;
      v3(0) = 1.;
      v3(1) = 3.;
      tria->set_boundary(1,*boundary1);
      tria->set_boundary(2,*boundary2);
      tria->begin_active()->face(1)->set_boundary_indicator(1);
      tria->begin_active()->face(2)->set_boundary_indicator(2);
      double pi=acos(-1);
      double alpha=2*atan(0.5);
      exact_areas.push_back(4+pi-2.5*(alpha-sin(alpha)));
      for (unsigned int i=0; i<=4; ++i)
	show[2][i]=1;
    }

  if (3)
    {
      tria=new Triangulation<2>();
      tria_ptr.push_back(tria);
      Point<2> p0;
      Point<2> p1;
      p0(0) = 1.;
      p0(1) = 2.5;
      p1(0) = 2.;
      p1(1) = 4.;
      GridGenerator::hyper_rectangle(*tria, p0, p1);
      exact_areas.push_back(1.5);
      show[3][5] = 1;
    }

  if (4 && false)
    {
      Boundary<2> *boundary1=new HyperBallBoundary<2>(Point<2>(2.5,-0.5),
						      sqrt(1.5*1.5+0.5*0.5));
      boundary_ptr.push_back(boundary1);
      tria=new Triangulation<2>();
      tria_ptr.push_back(tria);
      GridGenerator::hyper_cube(*tria, 0., 1.);
      Point<2> &v2 = tria->begin_active()->vertex(2);
      Point<2> &v3 = tria->begin_active()->vertex(3);
      v2(0) = 2.;
      v2(1) = 1.;
      v3(0) = 0.5;
      v3(1) = 1.5;
      tria->set_boundary(1,*boundary1);
      tria->begin_active()->face(1)->set_boundary_indicator(1);
      exact_areas.push_back(0.);
      for (unsigned int i=0; i<=4; ++i)
	show[4][i]=1;
    }
}



template<>
void create_triangulations(std::vector<Triangulation<3> *> &tria_ptr,
			   std::vector<Boundary<3> *> &boundary_ptr,
			   std::vector<double> &exact_areas)
{
  Triangulation<3> *tria;
  show.clear();
  show.resize(5, std::vector<unsigned int> (mapping_size,0));
  
				   // 2x2 cube
  if (1)
    {
      tria=new Triangulation<3>();
      tria_ptr.push_back(tria);
      GridGenerator::hyper_cube(*tria, 1., 3.);
      exact_areas.push_back(8.);
    }

				   // arbitrary quadrilateral
  if (1)
    {
      tria=new Triangulation<3>();
      tria_ptr.push_back(tria);
      GridGenerator::hyper_cube(*tria, 1., 3.);
      Point<3> &v=tria->begin()->vertex(6);
      v(0) = 5.;
      v(1) = 4.;
      v(2) = 4.5;
      exact_areas.push_back(12.5);
    }

				   // cube+part of ball
  if (2)
    {
      Point<3> m(2,2,2);
      Point<3> v(3,3,3);
      double r=sqrt((m-v).square()),
	     h=r-1.5,
	    pi=acos(-1);
      Boundary<3> *boundary1=new HyperBallBoundary<3>(m, r);
      boundary_ptr.push_back(boundary1);
      
      tria=new Triangulation<3>();
      tria_ptr.push_back(tria);
      GridGenerator::hyper_cube(*tria, 1., 3.);
      tria->set_boundary(1,*boundary1);
      tria->begin_active()->face(3)->set_boundary_indicator(1);
      exact_areas.push_back(8.+pi/3*h*h*(3*r-h));
    }

				   // eighth of ball
  if (3)
    {
      Point<3> p(0,0,0);
      const double r=sqrt(3.);
      Boundary<3> *boundary0=new HyperBallBoundary<3>(p, r);
      boundary_ptr.push_back(boundary0);

      tria=new Triangulation<3>();
      tria_ptr.push_back(tria);
      GridGenerator::hyper_cube(*tria, -1, 1.);
      tria->set_boundary(0, *boundary0);
      tria->refine_global(1);
      const double pi=acos(-1);
      exact_areas.push_back(4/3.*pi*r*r*r/8.);
      for (unsigned int i=0; i<4; ++i)
	show[3][i]=1;
    }
  if (4)
    {
      tria=new Triangulation<3>();
      tria_ptr.push_back(tria);
      Point<3> p0;
      Point<3> p1;
      p0(0) = 1.;
      p0(1) = 2.5;
      p0(2) = 3.;
      p1(0) = 2.;
      p1(1) = 4.;
      p1(2) = 6.;
      GridGenerator::hyper_rectangle(*tria, p0, p1);
      exact_areas.push_back(4.5);
      show[4][5] = 1;
    }
}

  
template<int dim>
void mapping_test()
{
  deallog << "dim=" << dim << std::endl;
  
  std::vector<Mapping<dim> *> mapping_ptr;
  std::vector<std::string> mapping_strings;

  MappingCartesian<dim> cart;
  MappingQ1<dim> q1_old;
  MappingQ<dim> q1(1);
  MappingQ<dim> q2(2);
  MappingQ<dim> q3(3);
//  MappingQ<dim> q4(4);
  mapping_ptr.push_back(&q1_old);
  mapping_ptr.push_back(&q1);
  mapping_ptr.push_back(&q2);
  mapping_ptr.push_back(&q3);
//  mapping_ptr.push_back(&q4);
  mapping_ptr.push_back(&cart);
  mapping_strings.push_back("Q1fixed");
  mapping_strings.push_back("Q1");
  mapping_strings.push_back("Q2");
  mapping_strings.push_back("Q3");
//  mapping_strings.push_back("Q4");
  mapping_strings.push_back("Cartesian");

  mapping_size=mapping_ptr.size();
  
  std::vector<Triangulation<dim> *> tria_ptr;
  std::vector<Boundary<dim> *> boundary_ptr;
  std::vector<double> exact_areas;
  
  create_triangulations(tria_ptr, boundary_ptr, exact_areas);
  Assert(show.size()==tria_ptr.size(), ExcInternalError());

  FE_Q<dim> fe_q4(4);
  
  for (unsigned int i=0; i<tria_ptr.size(); ++i)
    {
      DoFHandler<dim> dof(*tria_ptr[i]);
      dof.distribute_dofs(fe_q4);      
      typename DoFHandler<dim>::cell_iterator cell = dof.begin_active();
      
      deallog << "Triangulation" << i << ":" << std::endl;

      deallog << "exact_area=" << exact_areas[i] << std::endl;
      for (unsigned int j=0; j<mapping_size; ++j)
	if (show[i][j])
	  {
	    if (true)
	      {
		SSTREAM ost;
		ost << "Mapping" << dim << "d-" << i << '-'
		    << mapping_strings[j] ENDSTRING;
		deallog << ost.str() << std::endl;
		plot_transformation(*mapping_ptr[j], fe_q4, cell, ost.str());
		compute_area(*mapping_ptr[j], fe_q4, cell);
	      }
	    
	    if (dim>1)
	      {
		SSTREAM ost;
		ost << "MappingFace" << dim << "d-" << i << '-'
		    << mapping_strings[j] ENDSTRING;
		deallog << ost.str() << std::endl;	    
		plot_faces(*mapping_ptr[j], fe_q4, cell, ost.str());
	      }

	    if (dim>1)
	      {
		SSTREAM ost;
		ost << "MappingSubface" << dim << "d-" << i << '-'
		    << mapping_strings[j] ENDSTRING;
		deallog << ost.str() << std::endl;	    
		plot_subfaces(*mapping_ptr[j], fe_q4, cell, ost.str());
	      }

	    
				   // Test for transform_*_to_*_cell
	    if (true)
	      {
		Mapping<dim> &mapping=*mapping_ptr[j];
		Point<dim> p_unit;
		switch(dim)
		  {
		    case 1:
			  p_unit=Point<dim>(6/7.);
			  break;
		    case 2:
			  p_unit=Point<dim>(6/7.,4/7.);
			  break;
		    case 3:
			  p_unit=Point<dim>(6/7.,4/7.,5/7.);
			  break;
		  }
		
		Point<dim> p_real=mapping.transform_unit_to_real_cell(cell, p_unit);
		Point<dim> p_re_unit=mapping.transform_real_to_unit_cell(cell, p_real);
		deallog << "p_unit=" << p_unit << ",  p_real=" << p_real
			<< ",  p_re_unit=" << p_re_unit << std::endl;
	      }	    
	  }    
    }


				   // delete all triangulations and
				   // boundary objects
  for (unsigned int i=0; i<tria_ptr.size(); ++i)
    if (tria_ptr[i]!=0)
      delete tria_ptr[i];

  for (unsigned int i=0; i<boundary_ptr.size(); ++i)
    if (boundary_ptr[i]!=0)
      delete boundary_ptr[i];
}




int main()
{
  std::ofstream logfile ("mapping.output");
  logfile.precision (PRECISION);
  deallog.attach(logfile);
  deallog.depth_console(0);
  
				   // ----------------------- 
				   // Tests for dim=1
				   // -----------------------
  mapping_test<1>();

  
				   // ----------------------- 
				   // Tests for dim=2
				   // -----------------------
  mapping_test<2>();
  

				   // ----------------------- 
				   // Tests for dim=3
				   // -----------------------
  mapping_test<3>();
}

