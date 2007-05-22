/**
 * @file  vtkKWScubaLayer3DMRIS.cxx
 * @brief Displays 3D surfaces
 *
 * Displays surfaces as a 3D model.
 */
/*
 * Original Author: Kevin Teich
 * CVS Revision Info:
 *    $Author: kteich $
 *    $Date: 2007/05/22 22:05:15 $
 *    $Revision: 1.2 $
 *
 * Copyright (C) 2002-2007,
 * The General Hospital Corporation (Boston, MA). 
 * All rights reserved.
 *
 * Distribution, usage and copying of this software is covered under the
 * terms found in the License Agreement file named 'COPYING' found in the
 * FreeSurfer source code root directory, and duplicated here:
 * https://surfer.nmr.mgh.harvard.edu/fswiki/FreeSurferOpenSourceLicense
 *
 * General inquiries: freesurfer@nmr.mgh.harvard.edu
 * Bug reports: analysis-bugs@nmr.mgh.harvard.edu
 *
 */


#include <string>
#include <stdexcept>
#include "vtkKWScubaLayer3DMRIS.h"
#include "vtkCutter.h"
#include "vtkPlane.h"
#include "ScubaCollectionProperties.h"
#include "ScubaCollectionPropertiesMRIS.h"
#include "vtkFSSurfaceSource.h"
#include "vtkObjectFactory.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"
#include "vtkDecimatePro.h"
#include "vtkProperty.h"
#include "vtkTransformPolyDataFilter.h"
#include "vtkMatrix4x4.h"
#include "vtkTransform.h"
#include "vtkKWLabel.h"
#include "ScubaInfoItem.h"

using namespace std;

vtkStandardNewMacro( vtkKWScubaLayer3DMRIS );
vtkCxxRevisionMacro( vtkKWScubaLayer3DMRIS, "$Revision: 1.2 $" );

vtkKWScubaLayer3DMRIS::vtkKWScubaLayer3DMRIS () :
  mMRISProperties( NULL ),
  m3DNormalMapper( NULL ),
  m3DFastMapper( NULL ),
  m3DActor( NULL ) {
  mRASCenter[0] = mRASCenter[1] = mRASCenter[2] = 0;
	for( int n = 0; n < 3; n++ ) {
		m2DSlicePlane[n] = NULL;
		m2DNormalMapper[n] = NULL; 
		m2DFastMapper[n] = NULL;
		m2DActor[n] = NULL;
		
	}
}

vtkKWScubaLayer3DMRIS::~vtkKWScubaLayer3DMRIS () {

  if ( m3DNormalMapper )
    m3DNormalMapper->Delete();
  if ( m2DFastMapper )
    m3DFastMapper->Delete();
  if ( m2DActor )
    m3DActor->Delete();
	for( int n = 0; n < 3; n++ ) {
	  if( m2DSlicePlane[n] )
			m2DSlicePlane[n]->Delete();
	  if( m2DNormalMapper[n] )
			m2DNormalMapper[n]->Delete();
		if( m2DFastMapper[n] )
			m2DFastMapper[n]->Delete();
		if( m2DActor[n] ) 
			m2DActor[n]->Delete();
	}
}

void
vtkKWScubaLayer3DMRIS::SetMRISProperties ( ScubaCollectionPropertiesMRIS* const iProperties ) {
  mMRISProperties = iProperties;
}


void
vtkKWScubaLayer3DMRIS::Create () {

  // Bail if we don't have our source yet.
  if( NULL == mMRISProperties )
    throw runtime_error( "vtkKWScubaLayer3DMRIS::Create: No source" );
  
  //
  // Source object reads the surface and outputs poly data.
  //
  vtkFSSurfaceSource* source = mMRISProperties->GetSource();

  // Get some info from the MRIS.
  mRASCenter[0] = source->GetRASCenterX();
  mRASCenter[1] = source->GetRASCenterY();
  mRASCenter[2] = source->GetRASCenterZ();

  //
  // Mappers for the 3D surface.
  //
  m3DNormalMapper = vtkPolyDataMapper::New();
  m3DNormalMapper->SetInput( mMRISProperties->GetNormalModeOutput() );

  m3DFastMapper = vtkPolyDataMapper::New();
  m3DFastMapper->SetInput( mMRISProperties->GetFastModeOutput() );

  //
  // Actor for the 3D surface.
  //
  m3DActor = vtkActor::New();
  m3DActor->SetMapper( m3DNormalMapper );

  this->AddProp( m3DActor );

  // Update the fastClipper now so we do the decimation up front.
  m3DFastMapper->Update();


	// Now make three actors for the optional 2D plane intersection mode.
	for( int n = 0; n < 3; n++ ) {

		//
		// Cutting planes for the 2D intersections.
	  //
	  m2DSlicePlane[n] = vtkPlane::New();
	  m2DSlicePlane[n]->SetOrigin( mRASCenter[0], mRASCenter[1], mRASCenter[2] );
		m2DSlicePlane[n]->SetNormal( (n==0), (n==1), (n==2) );
	 
	  //
	  // Cutters that takes the 3D surface and outputs 2D lines.
	  //
	  vtkCutter* clipper = vtkCutter::New();
	  clipper->SetInputConnection( mMRISProperties->GetNormalModeOutputPort() );
	  clipper->SetCutFunction( m2DSlicePlane[n] );

	  vtkCutter* fastClipper = vtkCutter::New();
	  fastClipper->SetInputConnection( mMRISProperties->GetFastModeOutputPort() );
	  fastClipper->SetCutFunction( m2DSlicePlane[n] );

	  //
	  // Mappers for the lines.
	  //
	  m2DNormalMapper[n] = vtkPolyDataMapper::New();
	  m2DNormalMapper[n]->SetInput( clipper->GetOutput() );

	  m2DFastMapper[n] = vtkPolyDataMapper::New();
	  m2DFastMapper[n]->SetInput( fastClipper->GetOutput() );

	  //
	  // Actors in the scene, drawing the mapped lines.
	  //
	  m2DActor[n] = vtkActor::New();
	  m2DActor[n]->SetMapper( m2DNormalMapper[n] );
	  m2DActor[n]->SetBackfaceProperty( m2DActor[n]->MakeProperty() );
	  m2DActor[n]->GetBackfaceProperty()->BackfaceCullingOff();
	  m2DActor[n]->SetProperty( m2DActor[n]->MakeProperty() );
	  m2DActor[n]->GetProperty()->SetColor( 1, 0, 0 );
	  m2DActor[n]->GetProperty()->SetEdgeColor( 1, 0, 0 );
	  m2DActor[n]->GetProperty()->SetInterpolationToFlat();

	  this->AddProp( m2DActor[n] );
	}
	
}

void
vtkKWScubaLayer3DMRIS::AddControls ( vtkKWWidget* iPanel ) {

}

void
vtkKWScubaLayer3DMRIS::RemoveControls () {

}

void
vtkKWScubaLayer3DMRIS::DoListenToMessage ( string const isMessage,
					   void* const iData ) {

  if( isMessage == "OpacityChanged" ) {

    if ( m3DActor )
      if ( m3DActor->GetProperty() ) {
				m3DActor->GetProperty()->SetOpacity( mProperties->GetOpacity() );
				this->PipelineChanged();
      }

  } else if( isMessage == "FastModeChanged" ) {
    
//     if ( mProperties->GetFastMode() )
//       m3DActor->SetMapper( m3DFastMapper );
//     else
//       m3DNormalMapper->SetMapper( m3DNormalMapper );
    
//     this->PipelineChanged();

  } else if( isMessage == "Layer3DInfoChanged" ) {

    if ( m2DSlicePlane[0] && m2DSlicePlane[1] && m2DSlicePlane[2] ) {

		  float rasX = mViewProperties->Get3DRASX();
		  float rasY = mViewProperties->Get3DRASY();
		  float rasZ = mViewProperties->Get3DRASZ();
     
      m2DSlicePlane[0]->SetOrigin( rasX - mRASCenter[0], 0, 0 );
			m2DSlicePlane[0]->SetNormal( 1, 0, 0 );
      
     	m2DSlicePlane[1]->SetOrigin( 0, rasY - mRASCenter[1], 0 );
			m2DSlicePlane[1]->SetNormal( 0, 1, 0 );
 
  		m2DSlicePlane[2]->SetOrigin( 0, 0, rasZ - mRASCenter[2] );
			m2DSlicePlane[2]->SetNormal( 0, 0, 1 );

     this->PipelineChanged();
    }


	}
}

void
vtkKWScubaLayer3DMRIS::GetRASBounds ( float ioBounds[6] ) const {

  if ( mMRISProperties )
    mMRISProperties->GetSource()->GetRASBounds( ioBounds );
  else {
    for ( int nBound = 0; nBound < 6; nBound++ )
      ioBounds[nBound] = 0;
  }
}

void
vtkKWScubaLayer3DMRIS::GetInfoItems ( float iRAS[3],
                                      list<ScubaInfoItem>& ilInfo ) const {

  ScubaInfoItem info;
  char sLabel[1024], sValue[1024];

  try {
    float distance;
    int nVertex =
      mMRISProperties->GetSource()->FindVertexAtSurfaceRAS( iRAS, &distance );

    sprintf( sLabel, "%s vno", mProperties->GetLabel() );
    sprintf( sValue, "%d (%.2f)", nVertex, distance );
    info.Clear();
    info.SetLabel( sLabel );
    info.SetValue( sValue );
    info.SetShortenHint( false );
  } catch (...) {

    // No vertex found, don't worry.
    sprintf( sLabel, "%s vno", mProperties->GetLabel() );
    strncpy( sValue, "NONE", sizeof(sValue) );
    info.Clear();
    info.SetLabel( sLabel );
    info.SetValue( sValue );
    info.SetShortenHint( false );
  }

  // Return it.
  ilInfo.push_back( info );
}

