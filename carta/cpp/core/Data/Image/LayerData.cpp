#include "Grid/DataGrid.h"
#include "Contour/DataContours.h"
#include "DataSource.h"
#include "Data/Image/Draw/DrawSynchronizer.h"
#include "Data/DataLoader.h"
#include "Data/Util.h"
#include "Data/Colormap/ColorState.h"
#include "Data/Image/Grid/AxisMapper.h"
#include "Data/Image/Grid/LabelFormats.h"
#include "State/UtilState.h"

#include "CartaLib/PixelPipeline/CustomizablePixelPipeline.h"
#include "CartaLib/IWcsGridRenderService.h"
#include "CartaLib/AxisDisplayInfo.h"
#include "../../ImageRenderService.h"

#include <QDebug>
#include <QTime>
#include "LayerData.h"

using Carta::Lib::AxisInfo;
using Carta::Lib::AxisDisplayInfo;

namespace Carta {

namespace Data {

const QString LayerData::CLASS_NAME = "LayerData";
const QString LayerData::MASK = "mask";
const QString LayerData::LAYER_COLOR="colorSupport";
const QString LayerData::LAYER_ALPHA="alphaSupport";


class LayerData::Factory : public Carta::State::CartaObjectFactory {

public:

    Carta::State::CartaObject * create (const QString & path, const QString & id)
    {
        return new LayerData(path, id);
    }
};
bool LayerData::m_registered =
        Carta::State::ObjectManager::objectManager()->registerClass (CLASS_NAME,
                                                   new LayerData::Factory());


LayerData::LayerData(const QString& path, const QString& id) :
    Layer( CLASS_NAME, path, id),
    m_dataSource( new DataSource()),
    m_drawSync( nullptr ),
    m_stateColor( nullptr ){

        m_renderQueued = false;

        _initializeState();

        Carta::State::ObjectManager* objMan = Carta::State::ObjectManager::objectManager();
        ColorState* colorObj = objMan->createObject<ColorState>();
        m_stateColor.reset( colorObj );
        connect( m_stateColor.get(), SIGNAL( colorStateChanged()), this, SLOT(_colorChanged()));


        DataGrid* gridObj = objMan->createObject<DataGrid>();
        m_dataGrid.reset( gridObj );
        m_dataGrid->_initializeGridRenderer();
        _colorChanged();

        std::shared_ptr<Carta::Lib::IWcsGridRenderService> gridService = m_dataGrid->_getRenderer();
        std::shared_ptr<Carta::Core::ImageRenderService::Service> imageService = m_dataSource->_getRenderer();

        // create the synchronizer
        m_drawSync.reset( new DrawSynchronizer( imageService, gridService, this ) );


        // connect its done() slot to our renderingSlot()
        connect( m_drawSync.get(), & DrawSynchronizer::done,
                         this, & LayerData::_renderingDone );

}

void LayerData::_addContourSet( std::shared_ptr<DataContours> contour ){
    if ( contour ){
        QString targetName = contour->getName();
        std::shared_ptr<DataContours> contourSet = _getContour( targetName );
        if ( !contourSet ){
            m_dataContours.insert( contour );
        }
    }
}

void LayerData::_clearColorMap(){
    if ( m_stateColor ){
       disconnect( m_stateColor.get() );
       Carta::State::ObjectManager* objMan = Carta::State::ObjectManager::objectManager();
       objMan->removeObject( m_stateColor->getId() );
    }
}

void LayerData::_colorChanged(){
    _updateColor();
    Layer::_colorChanged();
}


void LayerData::_displayAxesChanged(std::vector<AxisInfo::KnownType> displayAxisTypes,
        const std::vector<int>& frames ){
    if ( m_dataSource ){
        m_dataSource->_setDisplayAxes( displayAxisTypes, frames );
    }
}




Carta::Lib::AxisInfo::KnownType LayerData::_getAxisType( int index ) const {
    AxisInfo::KnownType type = AxisInfo::KnownType::OTHER;
    if ( m_dataSource ){
        type = m_dataSource->_getAxisType( index );
    }
    return type;
}

AxisInfo::KnownType LayerData::_getAxisXType() const {
    AxisInfo::KnownType axisType = AxisInfo::KnownType::OTHER;
    if ( m_dataSource ){
        axisType = m_dataSource->_getAxisXType();
    }
    return axisType;
}

AxisInfo::KnownType LayerData::_getAxisYType() const {
    AxisInfo::KnownType axisType = AxisInfo::KnownType::OTHER;
    if ( m_dataSource ){
        axisType = m_dataSource->_getAxisYType();
    }
    return axisType;
}

std::vector<AxisInfo::KnownType> LayerData::_getAxisZTypes() const {
    std::vector<AxisInfo::KnownType> axisTypes;
    if ( m_dataSource ){
        axisTypes = m_dataSource->_getAxisZTypes();
    }
    return axisTypes;
}

std::vector<AxisInfo::KnownType> LayerData::_getAxisTypes() const {
    std::vector<AxisInfo::KnownType> axisTypes;
    if ( m_dataSource ){
        axisTypes = m_dataSource->_getAxisTypes();
    }
    return axisTypes;
}


QPointF LayerData::_getCenterPixel() const {
    QPointF center = QPointF( nan(""), nan("") );
    if ( m_dataSource ){
        center = m_dataSource->_getCenter();
    }
    return center;
}

std::shared_ptr<ColorState> LayerData::_getColorState(){
    return m_stateColor;
}

std::vector< std::shared_ptr<ColorState> >  LayerData::_getSelectedColorStates( bool global){
    std::vector< std::shared_ptr<ColorState> > colorStates;
    if ( _isSelected() || global ){
        colorStates.push_back( m_stateColor );
    }
    return colorStates;
}


std::shared_ptr<DataContours> LayerData::_getContour( const QString& name ){
    std::shared_ptr<DataContours> contourSet;
    for ( std::set<std::shared_ptr<DataContours> >::iterator it= m_dataContours.begin();
            it != m_dataContours.end(); it++ ){
        if ( name == (*it)->getName() ){
            contourSet = (*it);
            break;
        }
    }
    return contourSet;
}

std::set<std::shared_ptr<DataContours>> LayerData::_getContours() {
    return m_dataContours;
}

QStringList LayerData::_getCoordinates( double x, double y,
        Carta::Lib::KnownSkyCS system, const std::vector<int>& frames ) const{
    QStringList coordStr;
    if ( m_dataSource ){
        coordStr = m_dataSource->_getCoordinates( x, y, system, frames );
    }
    return coordStr;
}

Carta::Lib::KnownSkyCS LayerData::_getCoordinateSystem() const {
    Carta::Lib::KnownSkyCS cs = Carta::Lib::KnownSkyCS::Unknown;
    if ( m_dataGrid ){
        cs = m_dataGrid->_getSkyCS();
    }
    return cs;
}

QString LayerData::_getCursorText( int mouseX, int mouseY, const std::vector<int>& frames ){
    QString cursorText;
    if ( m_dataSource ){
        Carta::Lib::KnownSkyCS cs = m_dataGrid->_getSkyCS();
        cursorText = m_dataSource->_getCursorText( mouseX, mouseY, cs, frames );
    }
    return cursorText;
}

std::shared_ptr<DataSource> LayerData::_getDataSource(){
    return m_dataSource;
}


int LayerData::_getDimension( int coordIndex ) const {
    int dim = -1;
    if ( m_dataSource ){
        dim = m_dataSource->_getDimension( coordIndex );
    }
    return dim;
}


int LayerData::_getDimension() const {
    int imageSize = 0;
    if ( m_dataSource ){
        imageSize = m_dataSource->_getDimensions();
    }
    return imageSize;
}

std::vector<int> LayerData::_getImageDimensions( ) const {
    std::vector<int> result;
    if ( m_dataSource ){
        int dimensions = m_dataSource->_getDimensions();
        for ( int i = 0; i < dimensions; i++ ) {
            int d = m_dataSource->_getDimension( i );
            result.push_back( d );
        }
    }
    return result;
}


int LayerData::_getFrameCount( AxisInfo::KnownType type ) const {
    int frameCount = 1;
    if ( m_dataSource ){
        frameCount = m_dataSource->_getFrameCount( type );
    }
    return frameCount;
}

Carta::State::StateInterface LayerData::_getGridState() const {
    return m_dataGrid->_getState();
}

std::shared_ptr<Carta::Lib::Image::ImageInterface> LayerData::_getImage(){
    std::shared_ptr<Carta::Lib::Image::ImageInterface> image;
    if ( m_dataSource ){
        image = m_dataSource->_getImage();
    }
    return image;
}


QPointF LayerData::_getImagePt( QPointF screenPt, bool* valid ) const {
    QPointF imagePt;
    if ( m_dataSource ){
        imagePt = m_dataSource->_getImagePt( screenPt, valid );
    }
    else {
        *valid = false;
    }
    return imagePt;
}


bool LayerData::_getIntensity( int frameLow, int frameHigh, double percentile,
        double* intensity, int* intensityIndex ) const {
    bool intensityFound = false;
    if ( m_dataSource ){
        intensityFound = m_dataSource->_getIntensity( frameLow, frameHigh, percentile,
                intensity, intensityIndex );
    }
    return intensityFound;
}


float LayerData::_getMaskAlpha() const {
    QString key = Carta::State::UtilState::getLookup( MASK, Util::ALPHA );
    float maskInt = m_state.getValue<int>( key );
    float mask = maskInt / Util::MAX_COLOR;
    return mask;
}

quint32 LayerData::_getMaskColor() const {
    QString redKey = Carta::State::UtilState::getLookup( MASK, Util::RED );
    int redColor = m_state.getValue<int>( redKey );
    QString greenKey = Carta::State::UtilState::getLookup( MASK, Util::GREEN );
    int greenColor = m_state.getValue<int>( greenKey );
    QString blueKey = Carta::State::UtilState::getLookup( MASK, Util::BLUE );
    int blueColor = m_state.getValue<int>( blueKey );
    QRgb rgbCol = qRgba( redColor, greenColor, blueColor, 255 );
    return rgbCol;
}

QSize LayerData::_getOutputSize() const {
    QSize size;
    if ( m_dataSource ){
        size = m_dataSource-> _getOutputSize();
    }
    return size;
}

double LayerData::_getPercentile( int frameLow, int frameHigh, double intensity ) const {
    double percentile = 0;
    if ( m_dataSource ){
        percentile = m_dataSource->_getPercentile( frameLow, frameHigh, intensity );
    }
    return percentile;
}



QStringList LayerData::_getPixelCoordinates( double ra, double dec ) const{
    QStringList result("");
    if ( m_dataSource ){
        result = m_dataSource->_getPixelCoordinates( ra, dec );
    }
    return result;
}

QString LayerData::_getPixelUnits() const {
    QString units;
    if ( m_dataSource ){
        units = m_dataSource->_getPixelUnits();
    }
    return units;
}

QString LayerData::_getPixelValue( double x, double y, const std::vector<int>& frames ) const {
    QString pixelValue = "";
    if ( m_dataSource ){
        pixelValue = m_dataSource->_getPixelValue( x, y, frames );
    }
    return pixelValue;
}

QSize LayerData::_getSaveSize( const QSize& outputSize,  Qt::AspectRatioMode aspectMode) const {
    QSize saveSize = outputSize;

    //Get the grid margin space.
    int leftMargin = m_dataGrid->_getMargin( LabelFormats::EAST );
    int rightMargin = m_dataGrid->_getMargin( LabelFormats::WEST );
    int topMargin = m_dataGrid->_getMargin( LabelFormats::NORTH );
    int bottomMargin = m_dataGrid->_getMargin( LabelFormats::SOUTH );

    //Get the image dimensions
    Carta::Lib::AxisInfo::KnownType axisXType = m_dataSource->_getAxisXType();
    Carta::Lib::AxisInfo::KnownType axisYType = m_dataSource->_getAxisYType();
    int imageWidth = m_dataSource->_getFrameCount( axisXType );
    int imageHeight = m_dataSource->_getFrameCount( axisYType );


    if ( aspectMode == Qt::KeepAspectRatio || aspectMode == Qt::KeepAspectRatioByExpanding ){
        //Get the width and height after grid margins are subtracted off.
        int outWidth = outputSize.width() - leftMargin - rightMargin;
        int outHeight = outputSize.height() - topMargin - bottomMargin;

        //Get the image dimensions
        if ( m_dataSource ){
            double widthRatio = (outWidth*1.0) / imageWidth;
            double heightRatio = (outHeight*1.0) / imageHeight;
            double ratio = 1;
            if ( aspectMode == Qt::KeepAspectRatio ){
                ratio = qMin( widthRatio, heightRatio );
            }
            else {
                ratio = qMax( widthRatio, heightRatio );
            }
            double saveWidth = ratio * imageWidth + leftMargin + rightMargin;
            double saveHeight = ratio * imageHeight + topMargin + bottomMargin;
            saveSize = QSize( saveWidth, saveHeight );
        }
    }
    else {
        //Just save to the size specified by the user, unless it is too small.
        //If the user specified size is too small, produce an image of minimum size.
        double minWidth = imageWidth +leftMargin + rightMargin;
        double minHeight = imageHeight + topMargin + bottomMargin;
        if ( saveSize.width() < minWidth ){
            saveSize.setWidth( minWidth );
        }
        if ( saveSize.height() < minHeight ){
            saveSize.setHeight( minHeight );
        }
    }
    return saveSize;
}

QPointF LayerData::_getScreenPt( QPointF imagePt, bool* valid ) const {
    QPointF screenPt;
    if ( m_dataSource ){
        screenPt = m_dataSource->_getScreenPt( imagePt, valid );
    }
    else {
        *valid = false;
    }
    return screenPt;
}

QString LayerData::_getStateString( bool truncatePaths ) const{
    Carta::State::StateInterface copyState( m_state );
    if ( !truncatePaths ){
        copyState.setValue<QString>(Util::NAME, m_dataSource->_getFileName());
        copyState.insertObject( DataGrid::GRID, m_dataGrid->_getState().toString() );
        int contourCount = m_dataContours.size();
        copyState.insertArray( DataContours::CONTOURS, contourCount );
        int i = 0;
        for ( std::set< std::shared_ptr<DataContours> >::iterator iter = m_dataContours.begin();
            iter != m_dataContours.end(); iter++ ){
            QString lookup = Carta::State::UtilState::getLookup( DataContours::CONTOURS, i );
            copyState.setObject( lookup, (*iter)->_getState().toString() );
            i++;
        }
        QString colorState( "");
        if ( m_stateColor ){
            colorState =  m_stateColor->getStateString("", SNAPSHOT_PREFERENCES );
        }
        copyState.insertObject( ColorState::CLASS_NAME, colorState );
    }
    QString stateStr = copyState.toString();
    return stateStr;
}

double LayerData::_getZoom() const {
    double zoom = DataSource::ZOOM_DEFAULT;
    if ( m_dataSource ){
        zoom = m_dataSource-> _getZoom();
    }
    return zoom;
}

void LayerData::_gridChanged( const Carta::State::StateInterface& state ){
    m_dataGrid->_resetState( state );
}


void LayerData::_initializeState() {
    //Color mix
    m_state.insertObject( MASK );
    QString redKey = Carta::State::UtilState::getLookup( MASK, Util::RED );
    m_state.insertValue<int>( redKey, 255 );
    QString greenKey = Carta::State::UtilState::getLookup( MASK, Util::GREEN );
    m_state.insertValue<int>( greenKey, 255 );
    QString blueKey = Carta::State::UtilState::getLookup( MASK, Util::BLUE );
    m_state.insertValue<int>( blueKey, 255 );
    QString alphaKey = Carta::State::UtilState::getLookup( MASK, Util::ALPHA );
    m_state.insertValue<int>( alphaKey, 255 );
    QString layerColorKey = Carta::State::UtilState::getLookup( MASK, LAYER_COLOR );
    m_state.insertValue<bool>( layerColorKey, false );
    QString layerAlphaKey = Carta::State::UtilState::getLookup( MASK, LAYER_ALPHA );
    m_state.insertValue<bool>( layerAlphaKey, true );
}

bool LayerData::_isContourDraw() const {
    bool contourDraw = false;
    for ( std::set< std::shared_ptr<DataContours> >::iterator it = m_dataContours.begin();
            it != m_dataContours.end(); it++ ){
        if ( (*it)->isContourDraw() ){
            contourDraw = true;
            break;
        }
    }
    return contourDraw;
}

void LayerData::_load(std::vector<int> frames, bool recomputeClipsOnNewFrame,
        double minClipPercentile, double maxClipPercentile ){
    if ( m_dataSource ){
        m_dataSource->_load( frames, recomputeClipsOnNewFrame,
                minClipPercentile, maxClipPercentile );
        if ( m_dataGrid ){
            if ( m_dataGrid->_isGridVisible() ){
                std::shared_ptr<Carta::Lib::IWcsGridRenderService> gridService = m_dataGrid->_getRenderer();
                gridService->setInputImage( m_dataSource->_getImage() );
            }
        }
    }
}







void LayerData::_removeContourSet( std::shared_ptr<DataContours> contourSet ){
    if ( contourSet ){
        QString targetName = contourSet->getName();
        for ( std::set< std::shared_ptr<DataContours> >::iterator it = m_dataContours.begin();
                        it != m_dataContours.end(); it++ ){
            if ( targetName == (*it)->getName() ){
                m_dataContours.erase(*it);
                break;
            }
        }
    }
}


void LayerData::_renderingDone(
        QImage image,
        Carta::Lib::VectorGraphics::VGList gridVG,
        Carta::Lib::VectorGraphics::VGList contourVG,
        int64_t /*jobId*/){
    /// \todo we should make sure the jobId matches the last submitted job...
    Carta::Lib::VectorGraphics::VGList vectorGraphics;
    QImage qImage;
    if ( !image.isNull()){
        qImage = image;
        Carta::Lib::VectorGraphics::VGComposer comp = Carta::Lib::VectorGraphics::VGComposer( );
        if ( _isContourDraw()){
            // where does 0.5, 0.5 map to?
            if ( m_dataSource ){

                bool valid1 = false;
                QPointF p1 = m_dataSource->_getScreenPt( { 0.5, 0.5 }, &valid1 );

                // where does 1.5, 1.5 map to?
                bool valid2 = false;
                QPointF p2 = m_dataSource->_getScreenPt( { 1.5, 1.5 }, &valid2 );
                if ( valid1 && valid2 ){
                    QTransform tf;
                    double m11 = p2.x() - p1.x();
                    double m22 = p2.y() - p1.y();
                    double m33 = 1; // no projection
                    double m13 = 0; // no projection
                    double m23 = 0; // no projection
                    double m12 = 0; // no shearing
                    double m21 = 0; // no shearing
                    double m31 = p1.x() - m11 * 0.5;
                    double m32 = p1.y() - m22 * 0.5;
                    tf.setMatrix( m11, m12, m13, m21, m22, m23, m31, m32, m33 );

                    comp.append< Carta::Lib::VectorGraphics::Entries::Save >( );
                    comp.append< Carta::Lib::VectorGraphics::Entries::SetTransform >( tf );
                    comp.appendList( contourVG);
                    comp.append< Carta::Lib::VectorGraphics::Entries::Restore >( );
                }
            }
        }
        comp.appendList( gridVG);
        vectorGraphics = comp.vgList();
    }
    std::shared_ptr<RenderResponse> response( new RenderResponse(qImage, vectorGraphics, _getLayerId()) );
    emit renderingDone( response );

}

void LayerData::_renderStart(){

    m_renderQueued = true;

    //Get the render parameters from the next request.
    std::shared_ptr<RenderRequest> request = m_renderRequests.pop();
    std::vector<int> frames = request->getFrames();
    Carta::Lib::KnownSkyCS cs = request->getCoordinateSystem();
    bool topOfStack = request->isStackTop();
    QSize outputSize = request->getOutputSize();

    std::shared_ptr<Carta::Lib::IWcsGridRenderService> gridService = m_dataGrid->_getRenderer();
    std::shared_ptr<Carta::Core::ImageRenderService::Service> imageService = m_dataSource->_getRenderer();

    QSize renderSize = m_viewSize;
    if ( !outputSize.isNull() && !outputSize.isEmpty() &&
            outputSize.width() > 0 && outputSize.height() > 0  ){
        renderSize = outputSize;
    }

    m_dataSource->_viewResize( renderSize );
    gridService-> setOutputSize( renderSize );

    int leftMargin = m_dataGrid->_getMargin( LabelFormats::EAST );
    int rightMargin = m_dataGrid->_getMargin( LabelFormats::WEST );
    int topMargin = m_dataGrid->_getMargin( LabelFormats::NORTH );
    int bottomMargin = m_dataGrid->_getMargin( LabelFormats::SOUTH );
    int outWidth = renderSize.width() - leftMargin - rightMargin;
    int outHeight = renderSize.height() - topMargin - bottomMargin;
    QRectF outputRect( leftMargin, topMargin, outWidth, outHeight );

    QPointF topLeft = outputRect.topLeft();
    QPointF bottomRight = outputRect.bottomRight();

    QPointF topLeftInput = imageService-> screen2img( topLeft );
    QPointF bottomRightInput = imageService->screen2img( bottomRight );
    QRectF inputRect( topLeftInput, bottomRightInput );

    gridService-> setImageRect( inputRect );
    gridService-> setOutputRect( outputRect );

    std::vector<AxisDisplayInfo> axisInfo = m_dataSource->_getAxisDisplayInfo();
    int axisCount = axisInfo.size();
    for ( int i = 0; i < axisCount; i++ ){
        Carta::Lib::AxisInfo::KnownType type = axisInfo[i].getAxisType();
        int frame = axisInfo[i].getFrame();
        //Not one of the display frames so reset it with the current frame.
        if ( frame >= 0 ){
            frame = m_dataSource->_getFrameIndex( static_cast<int>(type), frames );
            axisInfo[i].setFrame( frame );
        }
    }
    gridService->setAxisDisplayInfo( axisInfo );

    std::shared_ptr<Carta::Lib::NdArray::RawViewInterface> rawData( m_dataSource->_getRawData( frames ));
    m_drawSync->setInput( rawData );
    m_drawSync->setContours( m_dataContours );

    //Which display axes will be drawn.
    AxisInfo::KnownType xType = m_dataSource->_getAxisXType();
    AxisInfo::KnownType yType = m_dataSource->_getAxisYType();
    QString displayLabelX = AxisMapper::getPurpose( xType, cs );
    QString displayLabelY = AxisMapper::getPurpose( yType, cs );
    gridService->setAxisLabel( 0, displayLabelX );
    gridService->setAxisLabel( 1, displayLabelY );

    AxisInfo::KnownType horAxisType = m_dataSource->_getAxisXType();
    Carta::Lib::AxisLabelInfo horAxisInfo = m_dataGrid->_getAxisLabelInfo( 0, horAxisType, cs );
    gridService->setAxisLabelInfo( 0, horAxisInfo );
    AxisInfo::KnownType vertAxisType = m_dataSource->_getAxisYType();
    Carta::Lib::AxisLabelInfo vertAxisInfo = m_dataGrid->_getAxisLabelInfo( 1, vertAxisType, cs );
    gridService->setAxisLabelInfo( 1, vertAxisInfo );

    bool contourDraw = _isContourDraw();
    bool gridDraw = false;
    if ( topOfStack ){
        gridDraw = m_dataGrid->_isGridVisible();
    }

    m_drawSync-> start( contourDraw, gridDraw );
}



void LayerData::_resetStateContours(const Carta::State::StateInterface& restoreState ){
    int contourCount = restoreState.getArraySize( DataContours::CONTOURS );
    Carta::State::ObjectManager* objMan = Carta::State::ObjectManager::objectManager();
    QStringList supportedContours;

    //Add any contours not there
    for ( int i = 0; i < contourCount; i++ ){
       QString lookup = Carta::State::UtilState::getLookup( DataContours::CONTOURS, i );
       QString nameLookup = Carta::State::UtilState::getLookup( lookup, Util::NAME );
       QString contourStr = restoreState.toString( lookup);
       QString contourName = restoreState.getValue<QString>( nameLookup );
       supportedContours.append( contourName );
       bool newContourSet = false;
       std::shared_ptr<DataContours> contourSet = _getContour( contourName );
       if ( !contourSet ){
           newContourSet = true;
          contourSet = std::shared_ptr<DataContours>(objMan->createObject<DataContours>());
          m_dataContours.insert( contourSet );
       }

       QString contourSetState = restoreState.toString( lookup );
       contourSet->resetState( contourSetState );
       if ( newContourSet ){
           emit contourSetAdded( this, contourName );
       }
   }

   //Remove any contours no longer there
    for ( std::set<std::shared_ptr<DataContours> >::iterator it = m_dataContours.begin();
            it != m_dataContours.end(); it++ ){
        QString contourSetName = (*it)->getName();
        if ( !supportedContours.contains( contourSetName )){
            _removeContourSet( (*it) );
            emit contourSetRemoved( contourSetName );
        }
    }
}

void LayerData::_resetState( const Carta::State::StateInterface& restoreState ){
    //Restore the other state variables
    Layer::_resetState( restoreState );

    //Restore the grid
    QString gridStr = restoreState.toString( DataGrid::GRID );
    Carta::State::StateInterface gridState( "" );
    gridState.setState( gridStr );
    _gridChanged( gridState);
    _resetStateContours( restoreState );
    QString colorState = restoreState.toString( ColorState::CLASS_NAME);
    if ( colorState.length() > 0 ){
        //Create a color state if it does not exist.
        if ( ! m_stateColor ){
            Carta::State::ObjectManager* objMan = Carta::State::ObjectManager::objectManager();
            ColorState* cObject = objMan->createObject<ColorState>();

            m_stateColor.reset( cObject);
            connect( m_stateColor.get(), SIGNAL( colorStateChanged()), this, SLOT(_colorChanged()));
        }
        m_stateColor->_resetState( colorState );
    }
    _updateColor();

    //Color mix
    QString redKey = Carta::State::UtilState::getLookup( MASK, Util::RED );
    m_state.setValue<int>( redKey, restoreState.getValue<int>(redKey) );
    QString greenKey = Carta::State::UtilState::getLookup( MASK, Util::GREEN );
    m_state.setValue<int>( greenKey, restoreState.getValue<int>(greenKey) );
    QString blueKey = Carta::State::UtilState::getLookup( MASK, Util::BLUE );
    m_state.setValue<int>( blueKey, restoreState.getValue<int>(blueKey) );
    QString alphaKey = Carta::State::UtilState::getLookup( MASK, Util::ALPHA );
    m_state.setValue<int>( alphaKey, restoreState.getValue<int>(alphaKey) );
    QString layerColorKey = Carta::State::UtilState::getLookup( MASK, LAYER_COLOR );
    m_state.setValue<bool>( layerColorKey, restoreState.getValue<bool>(layerColorKey) );
    QString layerAlphaKey = Carta::State::UtilState::getLookup( MASK, LAYER_ALPHA );
    m_state.setValue<bool>( layerAlphaKey, restoreState.getValue<bool>(layerAlphaKey) );
}


void LayerData::_resetZoom( ){
    if ( m_dataSource ){
        m_dataSource->_resetZoom();
    }
}


void LayerData::_resetPan( ){
    if ( m_dataSource ){
        m_dataSource->_resetPan();
    }
}


QString LayerData::_setFileName( const QString& fileName, bool * success ){
    QString result = m_dataSource->_setFileName( fileName, success );
    if ( *success){
        DataLoader* dLoader = Util::findSingletonObject<DataLoader>();
        QString shortName = dLoader->getShortName( fileName );
        //m_state.setValue<QString>(DataSource::DATA_PATH, fileName );

        //Default is to have the layer name match the file name, unless
        //the user has explicitly set it.
        QString layerName = m_state.getValue<QString>( LAYER_NAME );
        if ( layerName.isEmpty() || layerName.length() == 0 ){
            m_state.setValue<QString>( LAYER_NAME, shortName );
            m_state.flushState();
        }
        result = m_state.getValue<QString>( Util::ID );
    }
    return result;
}

bool LayerData::_setLayersGrouped( bool /*grouped*/  ){
    return false;
}


bool LayerData::_setMaskColor( const QString& id, int redAmount,
        int greenAmount, int blueAmount){
    bool changed = false;
    if ( id == _getLayerId() ){
        QString redKey = Carta::State::UtilState::getLookup( MASK, Util::RED );
        int oldRedAmount = m_state.getValue<int>( redKey );
        QString greenKey = Carta::State::UtilState::getLookup( MASK, Util::GREEN );
        int oldGreenAmount = m_state.getValue<int>( greenKey );
        QString blueKey = Carta::State::UtilState::getLookup( MASK, Util::BLUE );
        int oldBlueAmount = m_state.getValue<int>( blueKey );
        if ( redAmount != oldRedAmount || greenAmount != oldGreenAmount ||
                blueAmount != oldBlueAmount ){
            changed = true;
            m_state.setValue<int>( redKey, redAmount );
            m_state.setValue<int>( greenKey, greenAmount );
            m_state.setValue<int>( blueKey, blueAmount );
        }
    }
    return changed;
}

void LayerData::_setMaskColorDefault(){
    _setMaskColor( _getLayerId(), 255,255,255);
}


bool LayerData::_setMaskAlpha( const QString& id, int alphaAmount ){
    bool changed = false;
    if ( id == _getLayerId() ){
        QString lookup = Carta::State::UtilState::getLookup( MASK, Util::ALPHA );
        int oldAlpha = m_state.getValue<int>( lookup );
        if ( oldAlpha != alphaAmount ){
            changed = true;
            m_state.setValue<int>( lookup, alphaAmount );
        }
    }
    return changed;
}

void LayerData::_setMaskAlphaDefault(){
    _setMaskAlpha( _getLayerId(), 255);
}



void LayerData::_setPan( double imgX, double imgY ){
    if ( m_dataSource ){
        m_dataSource-> _setPan( imgX, imgY );
    }
}

void LayerData::_setSupportAlpha( bool supportAlpha ){
    QString layerAlphaKey = Carta::State::UtilState::getLookup( MASK, LAYER_ALPHA );
    m_state.setValue( layerAlphaKey , supportAlpha );
    if ( !supportAlpha ){
        _setMaskAlphaDefault();
    }
}

void LayerData::_setSupportColor( bool supportColor ){
    QString layerColorKey = Carta::State::UtilState::getLookup( MASK, LAYER_COLOR );
    m_state.setValue( layerColorKey , supportColor );
    if ( !supportColor ){
        _setMaskColorDefault();
    }
}


void LayerData::_setZoom( double zoomAmount){
    if ( m_dataSource ){
        m_dataSource-> _setZoom( zoomAmount );
    }
}


void LayerData::_updateClips( std::shared_ptr<Carta::Lib::NdArray::RawViewInterface>& view,
        double minClipPercentile, double maxClipPercentile, const std::vector<int>& frames ){
    if ( m_dataSource ){
        m_dataSource->_updateClips( view,  minClipPercentile, maxClipPercentile, frames );
    }
}

void LayerData::_updateColor(){
    if ( m_dataSource ){
        QString mapName = m_stateColor->_getColorMap();
        m_dataSource->_setColorMap( mapName );
        m_dataSource->_setTransformData( m_stateColor->_getDataTransform() );
        m_dataSource->_setGamma( m_stateColor->_getGamma() );
        bool reversed = m_stateColor->_isReversed();
        m_dataSource->_setColorReversed( reversed );
        m_dataSource->_setColorInverted( m_stateColor->_isInverted() );
        double redAmount = m_stateColor->_getMixRed();
        double greenAmount = m_stateColor->_getMixGreen();
        double blueAmount = m_stateColor->_getMixBlue();
        m_dataSource->_setColorAmounts( redAmount, greenAmount, blueAmount );
        bool defaultNan = m_stateColor->_isNanDefault();
        m_dataSource->_setNanDefault( defaultNan );

        //If we aren't using a default nan color, tell the dataSource to use
        //the custom color.
        if ( !defaultNan ){
            int redAmount = m_stateColor->_getNanRed();
            int greenAmount = m_stateColor->_getNanGreen();
            int blueAmount = m_stateColor->_getNanBlue();
            m_dataSource->_setColorNan( redAmount, greenAmount, blueAmount );
        }
        //We are using  the default nan color.  Update the state to the default color.
        else {
            QColor nanColor = m_dataSource->_getNanColor();
            m_stateColor->_setNanColor( nanColor.red(), nanColor.green(), nanColor.blue() );
        }

        int redBorder = m_stateColor->_getBorderRed();
        int blueBorder = m_stateColor->_getBorderBlue();
        int greenBorder = m_stateColor->_getBorderGreen();
        int alphaAmount = m_stateColor->_getBorderTransparency();
        m_dataGrid->_setBorderColor( QColor(redBorder, greenBorder, blueBorder, alphaAmount) );
        Layer::_updateColor();
    }
}

void LayerData::_viewResize( const QSize& newSize ){
    m_viewSize = newSize;
    if ( m_dataSource ){
        m_dataSource->_viewResize( newSize );
    }
}

void LayerData::_viewReset(){
    _viewResize( m_viewSize );
    _setPan( m_viewPan.x(), m_viewPan.y() );
}



LayerData::~LayerData() {
}
}
}
