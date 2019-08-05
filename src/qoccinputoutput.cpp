/****************************************************************************
**
** This file is part of the QtOPENCASCADE Toolkit.
**
** This file may be used under the terms of the GNU General Public
** License version 2.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of
** this file.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
** Copyright (C) 2006, 2007, Peter Dolbey.
**
** All rights reserved.
**
****************************************************************************/

/*
\class	QoccInputOutput
\brief	This class provides input/output elements of the QtOpenCascade Toolkit.
        Currently a "work-in-progress", based on the standard examples. The
        main lesson learnt here is that with Qt4 you cannot directly cast
        QString to a "char *", so to transform to a OCC Standard_CString use
        <pre>Standard_CString s = aQString.toLatin1().data() ;</pre>
\author	Peter C. Dolbey
*/

#include <QApplication>

#include <Poly_Triangulation.hxx>
#include <RWStl.hxx>
#include <OSD_Path.hxx>
#include <MeshVS_Mesh.hxx>
#include <MeshVS_Drawer.hxx>
#include <MeshVS_DrawerAttribute.hxx>
#include <MeshVS_MeshPrsBuilder.hxx>
#include <XSDRAWSTLVRML_DataSource.hxx>
#include <Graphic3d_MaterialAspect.hxx>

#include "qoccinternal.h"
#include "qoccinputoutput.h"

QoccInputOutput::QoccInputOutput(void)
{
}

QoccInputOutput::~QoccInputOutput(void)
{
}

bool QoccInputOutput::importMesh( const QString& fileName,
                                  const FileFormat format,
                                  const Handle_AIS_InteractiveContext& ic )
{
     //* STL Loader!
     OSD_Path thePath(fileName.toLatin1().data());
     Handle( Poly_Triangulation ) aSTLMesh = RWStl::ReadFile(thePath);
     Handle( MeshVS_Mesh  ) aMesh = new MeshVS_Mesh();
     Handle( XSDRAWSTLVRML_DataSource ) aDS = new XSDRAWSTLVRML_DataSource( aSTLMesh );

     aMesh->SetDataSource(aDS);
     aMesh->AddBuilder( new MeshVS_MeshPrsBuilder( aMesh), Standard_True );//False -> No selection 

     aMesh->GetDrawer()->SetBoolean(MeshVS_DA_DisplayNodes,Standard_False); //MeshVS_DrawerAttribute
     aMesh->GetDrawer()->SetBoolean(MeshVS_DA_ShowEdges,Standard_False);
     aMesh->GetDrawer()->SetMaterial(MeshVS_DA_FrontMaterial,Graphic3d_NOM_BRASS);


     aMesh->SetColor(Quantity_NOC_AZURE);
     aMesh->SetDisplayMode( MeshVS_DMF_Shading ); // Mode as defaut
     aMesh->SetHilightMode( MeshVS_DMF_WireFrame ); // Wireframe as default hilight mode

     ic->Display(aMesh,Standard_True);

     return true;
}

bool QoccInputOutput::importModel(  const QString& fileName,
                                    const FileFormat format,
                                    const Handle_AIS_InteractiveContext& ic )
{
    bool result;
    QApplication::setOverrideCursor( Qt::WaitCursor );
    if (format == FormatSTL)
    {
        
        result = importMesh( fileName, format, ic );
    }
    else
    {
        Handle_TopTools_HSequenceOfShape shapes = importModel( format, fileName );
        if ( shapes.IsNull() || !shapes->Length() )
        {
            result = false;
        }
        else
        {
            result = true;
            for ( int i = 1; i <= shapes->Length(); i++ )
            {
                Handle(AIS_Shape) anAISShape = new AIS_Shape( shapes->Value( i ) );
                ic->SetMaterial( anAISShape, Graphic3d_NOM_GOLD,Standard_True);
                ic->SetColor( anAISShape, Quantity_NOC_RED,Standard_True);
                ic->SetDisplayMode( anAISShape, 1, Standard_False );
                ic->Display(anAISShape, Standard_False);
            }
        }
    }
    QApplication::restoreOverrideCursor();
    // ic->UpdateCurrentViewer();
    return result;
}

Handle_TopTools_HSequenceOfShape QoccInputOutput::importModel( const FileFormat format, const QString& file )
{
    Handle_TopTools_HSequenceOfShape shapes;
    try {
        switch ( format )
        {
        case FormatBREP:
            shapes = importBREP( file );
            break;
        case FormatIGES:
            shapes = importIGES( file );
            break;
        case FormatSTEP:
            shapes = importSTEP( file );
            break;
        case FormatCSFDB:
            shapes = importCSFDB( file );
            break;
        default:
            // To Do - Error message here?
            break;
        }
    } catch ( Standard_Failure ) {
        shapes.Nullify();
    }
    return shapes;
}


/******************************************************************
 * EXPORT FUNCTIONALITY
 ******************************************************************/

bool QoccInputOutput::exportModel( const QString& fileName,
                                   const FileFormat format,
                                   const Handle_AIS_InteractiveContext& ic )
{

    Handle_TopTools_HSequenceOfShape shapes = getShapes( ic );
    if ( shapes.IsNull() || !shapes->Length() )
        return false;

    QApplication::setOverrideCursor( Qt::WaitCursor );
    bool stat = exportModel( format, fileName, shapes );
    QApplication::restoreOverrideCursor();
    return stat;
}

bool QoccInputOutput::exportModel( const FileFormat format, const QString& file, const Handle_TopTools_HSequenceOfShape& shapes )
{
    bool status;
    try {
        switch ( format )
        {
        case FormatBREP:
            status = exportBREP( file, shapes );
            break;
        case FormatIGES:
            status = exportIGES( file, shapes );
            break;
        case FormatSTEP:
            status = exportSTEP( file, shapes );
            break;
        case FormatCSFDB:
            status = exportCSFDB( file, shapes );
            break;
        case FormatSTL:
            status = exportSTL( file, shapes );
            break;
        case FormatVRML:
            status = exportVRML( file, shapes );
            break;
        }
    } catch ( Standard_Failure ) {
        status = false;
    }
    return status;
}

Handle_TopTools_HSequenceOfShape QoccInputOutput::getShapes( const Handle_AIS_InteractiveContext& ic )
{
    Handle_TopTools_HSequenceOfShape aSequence;
    Handle_AIS_InteractiveObject picked;
    for ( ic->InitCurrent(); ic->MoreCurrent(); ic->NextCurrent() )
    {
        Handle_AIS_InteractiveObject obj = ic->Current();
        if ( obj->IsKind( STANDARD_TYPE( AIS_Shape ) ) )
        {
            TopoDS_Shape shape = Handle_AIS_Shape::DownCast(obj)->Shape();
            if ( aSequence.IsNull() )
                aSequence = new TopTools_HSequenceOfShape();
            aSequence->Append( shape );
        }
    }
    return aSequence;
}


Handle_TopTools_HSequenceOfShape QoccInputOutput::importBREP( const QString& file )
{
    Handle_TopTools_HSequenceOfShape aSequence;
    TopoDS_Shape aShape;
    BRep_Builder aBuilder;

    Standard_Boolean result = BRepTools::Read(  aShape, file.toLatin1().data(), aBuilder );
    if ( result )
    {
        aSequence = new TopTools_HSequenceOfShape();
        aSequence->Append( aShape );
    }

    return aSequence;
}

Handle_TopTools_HSequenceOfShape QoccInputOutput::importIGES( const QString& file )
{
    Handle_TopTools_HSequenceOfShape aSequence;
    IGESControl_Reader Reader;
    int status = Reader.ReadFile( file.toLatin1().data() );

    if ( status == IFSelect_RetDone )
    {
        aSequence = new TopTools_HSequenceOfShape();
        Reader.TransferRoots();
        TopoDS_Shape aShape = Reader.OneShape();
        aSequence->Append( aShape );
    }
    return aSequence;
}

Handle_TopTools_HSequenceOfShape QoccInputOutput::importSTEP( const QString& file )
{
    Handle_TopTools_HSequenceOfShape aSequence;

    STEPControl_Reader aReader;
    IFSelect_ReturnStatus status = aReader.ReadFile( file.toLatin1().data() );
    if ( status == IFSelect_RetDone )
    {
        //Interface_TraceFile::SetDefault();
        bool failsonly = false;
        aReader.PrintCheckLoad( failsonly, IFSelect_ItemsByEntity );

        int nbr = aReader.NbRootsForTransfer();
        aReader.PrintCheckTransfer( failsonly, IFSelect_ItemsByEntity );
        for ( Standard_Integer n = 1; n <= nbr; n++ )
        {
            bool ok = aReader.TransferRoot( n );
            if (ok)
            {
                int nbs = aReader.NbShapes();
                if ( nbs > 0 )
                {
                    aSequence = new TopTools_HSequenceOfShape();
                    for ( int i = 1; i <= nbs; i++ )
                    {
                        TopoDS_Shape shape = aReader.Shape( i );
                        aSequence->Append( shape );
                    }
                }
            }
        }
    }
    return aSequence;
}

Handle_TopTools_HSequenceOfShape QoccInputOutput::importCSFDB( const QString& file )
{
    Handle_TopTools_HSequenceOfShape aSequence;

#if 0
    // Check file type
    if ( FSD_File::IsGoodFileType( file.toLatin1().data() ) != Storage_VSOk )
        return aSequence;

    static FSD_File fileDriver;
    TCollection_AsciiString aName( file.toLatin1().data() );
    if ( fileDriver.Open( aName, Storage_VSRead ) != Storage_VSOk )
        return aSequence;

    Handle(ShapeSchema) schema = new ShapeSchema();
    Handle(Storage_Data) data  = schema->Read( fileDriver );
    if ( data->ErrorStatus() != Storage_VSOk )
        return aSequence;

    fileDriver.Close();

    aSequence = new TopTools_HSequenceOfShape();
    Handle(Storage_HSeqOfRoot) roots = data->Roots();
    for ( int i = 1; i <= roots->Length() ; i++ )
    {
        Handle(Storage_Root) r = roots->Value( i );
        Handle(Standard_Persistent) p = r->Object();
        Handle(PTopoDS_HShape) aPShape = Handle(PTopoDS_HShape)::DownCast(p);
        if ( !aPShape.IsNull() )
        {
            PTColStd_PersistentTransientMap aMap;
            TopoDS_Shape aTShape;
            MgtBRep::Translate( aPShape, aMap, aTShape, MgtBRep_WithTriangle );
            aSequence->Append( aTShape );
        }
    }
#endif
    return aSequence;
}

bool QoccInputOutput::exportBREP( const QString& file, const Handle_TopTools_HSequenceOfShape& shapes )
{
    if ( shapes.IsNull() || shapes->IsEmpty() )
        return false;

    TopoDS_Shape shape = shapes->Value( 1 );
    return BRepTools::Write( shape, file.toLatin1().data() );
}

bool QoccInputOutput::exportIGES( const QString& file, const Handle_TopTools_HSequenceOfShape& shapes )
{
    if ( shapes.IsNull() || shapes->IsEmpty() )
        return false;

    IGESControl_Controller::Init();
    IGESControl_Writer writer( Interface_Static::CVal( "XSTEP.iges.unit" ),
                               Interface_Static::IVal( "XSTEP.iges.writebrep.mode" ) );

    for ( int i = 1; i <= shapes->Length(); i++ )
        writer.AddShape ( shapes->Value( i ) );
    writer.ComputeModel();
    return writer.Write( file.toLatin1().data() );
}

bool QoccInputOutput::exportSTEP( const QString& file, const Handle_TopTools_HSequenceOfShape& shapes )
{
    if ( shapes.IsNull() || shapes->IsEmpty() )
        return false;

    IFSelect_ReturnStatus status;

    STEPControl_Writer writer;
    for ( int i = 1; i <= shapes->Length(); i++ )
    {
        status = writer.Transfer( shapes->Value( i ), STEPControl_AsIs );
        if ( status != IFSelect_RetDone )
            return false;
    }

    status = writer.Write( file.toLatin1().data() );

    switch ( status )
    {
    case IFSelect_RetError:
        myInfo = tr( "INF_DATA_ERROR" );
        break;
    case IFSelect_RetFail:
        myInfo = tr( "INF_WRITING_ERROR" );
        break;
    case IFSelect_RetVoid:
        myInfo = tr( "INF_NOTHING_ERROR" );
        break;
    default:
        break;
    }
    return status == IFSelect_RetDone;
}

bool QoccInputOutput::exportCSFDB( const QString& file, const Handle_TopTools_HSequenceOfShape& shapes )
{
#if 0
    if ( shapes.IsNull() || shapes->IsEmpty() )
        return false;

    static FSD_File fileDriver;

   // Handle(ShapeSchema) schema = new ShapeSchema();
    Handle(Storage_Data) data  = new Storage_Data();
    data->ClearErrorStatus();

    data->SetApplicationName( TCollection_ExtendedString( "Sample Import / Export" ) );
    data->SetApplicationVersion( "1" );
    data->SetDataType( TCollection_ExtendedString( "Shapes" ) );
    data->AddToUserInfo( "Storing a persistent set of shapes in a flat file" );
    data->AddToComments( TCollection_ExtendedString( "Application is based on CasCade 5.0 Professional" ) );

    if ( fileDriver.Open( file.toLatin1().data(), Storage_VSWrite ) != Storage_VSOk )
    {
        myInfo = tr( "INF_TRANSLATE_ERROR_CANTSAVEFILE" ).arg( file );
        return false;
    }

    PTColStd_TransientPersistentMap aMap;
    for ( int i = 1; i <= shapes->Length(); i++ )
    {
        TopoDS_Shape shape = shapes->Value( i );
        if ( shape.IsNull() )
        {
            myInfo = tr( "INF_TRANSLATE_ERROR_INVALIDSHAPE" );
            return false;
        }

        Handle(PTopoDS_HShape) pshape = MgtBRep::Translate( shape, aMap, MgtBRep_WithTriangle );
        TCollection_AsciiString objName = TCollection_AsciiString( "Object_" ) + TCollection_AsciiString( i );
        data->AddRoot( objName, pshape );
    }

    schema->Write( fileDriver, data );
    fileDriver.Close();

    if ( data->ErrorStatus() != Storage_VSOk )
    {
        myInfo = tr( "INF_TRANSLATE_ERROR_CANTSAVEDATA" );
        return false;
    }
#endif
    return true;
}

bool QoccInputOutput::exportSTL( const QString& file, const Handle_TopTools_HSequenceOfShape& shapes )
{
    if ( shapes.IsNull() || shapes->IsEmpty() )
        return false;

    TopoDS_Compound res;
    BRep_Builder builder;
    builder.MakeCompound( res );

    for ( int i = 1; i <= shapes->Length(); i++ )
    {
        TopoDS_Shape shape = shapes->Value( i );
        if ( shape.IsNull() )
        {
            myInfo = tr( "INF_TRANSLATE_ERROR_INVALIDSHAPE" );
            return false;
        }
        builder.Add( res, shape );
    }

    StlAPI_Writer writer;
    writer.Write( res, file.toLatin1().data() );

    return true;
}

bool QoccInputOutput::exportVRML( const QString& file, const Handle_TopTools_HSequenceOfShape& shapes )
{
    if ( shapes.IsNull() || shapes->IsEmpty() )
        return false;

    TopoDS_Compound res;
    BRep_Builder builder;
    builder.MakeCompound( res );

    for ( int i = 1; i <= shapes->Length(); i++ )
    {
        TopoDS_Shape shape = shapes->Value( i );
        if ( shape.IsNull() )
        {
            myInfo = tr( "INF_TRANSLATE_ERROR_INVALIDSHAPE" );
            return false;
        }
        builder.Add( res, shape );
    }

    VrmlAPI_Writer writer;
    writer.Write( res, file.toLatin1().data() );

    return true;
}

bool QoccInputOutput::checkFacetedBrep( const Handle_TopTools_HSequenceOfShape& shapes )
{
    bool err = false;
    for ( int i = 1; i <= shapes->Length(); i++ )
    {
        TopoDS_Shape shape = shapes->Value( i );
        for ( TopExp_Explorer fexp( shape, TopAbs_FACE ); fexp.More() && !err; fexp.Next() )
        {
            Handle(Geom_Surface) surface = BRep_Tool::Surface( TopoDS::Face( fexp.Current() ) );
            if ( !surface->IsKind( STANDARD_TYPE( Geom_Plane ) ) )
                err = true;
        }
        for ( TopExp_Explorer eexp( shape, TopAbs_EDGE ); eexp.More() && !err; eexp.Next() )
        {
            Standard_Real fd, ld;
            Handle(Geom_Curve) curve = BRep_Tool::Curve( TopoDS::Edge( eexp.Current() ), fd, ld );
            if ( !curve->IsKind( STANDARD_TYPE( Geom_Line ) ) )
                err = true;
        }
    }
    return !err;
}
