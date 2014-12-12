#ifndef PCLXL_H_INCLUDED
#define PCLXL_H_INCLUDED

/* This file defines elements of Hewlett Packard's PCL-XL printer
   control language (fka PCL-6)
*/

enum ArcDirection {
    eClockWise = 0,
    eCounterClockWise = 1
};

enum CharSubModeArray {
    eNoSubstitution = 0,
    eVerticalSubstitution = 1
};

enum ClipRegion {
    eInterior = 0,
    eExterior = 1
};

enum ColorDepth {
    e1Bit = 0,
    e4Bit = 1,
    e8Bit = 2
};

enum ColrMapping {
    eDirectPixel = 0,
    eIndexedPixel = 1
};

enum Colorspace {
    eGray = 1,
    eRGB = 2
};

enum Compression {
    eNoCompression = 0,
    eRLECompression = 1,
    eJPEGCompression = 2
};

enum DataOrg {
    eBinaryHighByteFirst = 0,
    eBinaryLowByteFirst = 1
};

enum DataSource {
    eDefault = 0
};

enum DataType {
    eUByte = 0,
    eSByte = 1,
    eUint16 = 2,
    eSint16 = 3
};

enum DitherMatrix {  
    eDeviceBest = 0
}; 


enum DuplexPageMode {
    eDuplexHorizontalBinding = 0,
    eDuplexVerticalBinding = 1
};

enum DuplexPageSide {
    eFrontMediaSide = 0,
    eBackMediaSide = 1
};

enum ErrorReport {
    eNoReporting = 0,
    eBackChannel = 1,
    eErrorPage = 2,
    eBackChAndErrPage = 3,
    eNWBackChannel = 4 ,
    eNWErrorPage = 5,
    eNWBackChAndErrPage = 6
};

enum FillMode {
    eNonZeroWinding = 0,
    eEvenOdd = 1
};

enum LineCap {
    eButtCap = 0,
    eRoundCap = 1,
    eSquareCap = 2,
    eTriangleCap = 3
};

enum LineJoin {
    eMiterJoin = 0,
    eRoundJoin = 1,
    eBevelJoin = 2,
    eNoJoin = 3
};

enum Measure {
    eInch = 0,
    eMillimeter = 1,
    eTenthsOfAMillimeter = 2
};

enum MediaSource {
    eDefaultSource = 0,
    eAutoSelect = 1,
    eManualFeed = 2,
    eMultiPurposeTray = 3,
    eUpperCassette = 4,
    eLowerCassette = 5,
    eEnvelopeTray = 6,
    eThirdCassette = 7
};

enum MediaDestination {
    eDefaultDestination = 0,
    eFaceDownBin = 1,
    eFaceUpBin = 2,
    eJobOffsetBin = 3
};

enum Orientation {
    ePortraitOrientation = 0,
    eLandscapeOrientation = 1,
    eReversePortrait = 2,
    eReverseLandscape = 3
};

enum PatternPersistence {
    eTempPattern = 0,
    ePagePattern = 1,
    eSessionPattern = 2
};

enum SimplexPageMode {
    eSimplexFrontSide = 0
};

enum TxMode {
    eOpaque = 0,
    eTransparent = 1
};

enum WritingMode{
    eHorizontal = 0,
    eVertical = 1
};

enum Attribute {
    aPaletteDepth        =   2,
    aColorSpace          =   3,
    aNullBrush           =   4,
    aNullPen             =   5,
    aPaletteData         =   6,
    aPatternSelectID     =   8,
    aGrayLevel           =   9,
    aRGBColor            =  11,
    aPatternOrigin       =  12,
    aNewDestinationSize  =  13,
    aPrimaryArray        =  14,
    aPrimaryDepth        =  15,
    aDeviceMatrix        =  33,
    aDitherMatrixDataType=  34,
    aDitherOrigin        =  35,
    aMediaDestination    =  36,
    aMediaSize           =  37,
    aMediaSource         =  38,
    aMediaType           =  39,
    aOrientation         =  40,
    aPageAngle           =  41,
    aPageOrigin          =  42,
    aPageScale           =  43,
    aROP3                =  44,
    aTxMode              =  45,
    aCustomMediaSize     =  47,
    aCustomMediaUnits    =  48,
    aPageCopies          =  49,
    aDitherMatrixSize    =  50,
    aDitherMatrixDepth   =  51,
    aSimplexPageMode     =  52,
    aDuplexPageMode      =  53,
    aDuplexPageSide      =  54,
    aArcDirection        =  65,
    aBoundingBox         =  66,
    aDashOffset          =  67,
    aEllipseDimension    =  68,
    aEndPoint            =  69,
    aFillMode            =  70,
    aLineCapStyle        =  71,
    aLineJoinStyle       =  72,
    aMiterLength         =  73,
    aLineDashStyle       =  74,
    aPenWidth            =  75,
    aPoint               =  76,
    aNumberOfPoints      =  77,
    aSolidLine           =  78,
    aStartPoint          =  79,
    aPointType           =  80,
    aControlPoint1       =  81,
    aControlPoint2       =  82,
    aClipRegion          =  83,
    aClipMode            =  84,
    aColorDepth          =  98,
    aBlockHeight         =  99,
    aColorMapping        = 100,
    aCompressMode        = 101,
    aDestinationBox      = 102,
    aDestinationSize     = 103,
    aPatternPersistence  = 104,
    aPatternDefineID     = 105,
    aSourceHeight        = 107,
    aSourceWidth         = 108,
    aStartLine           = 109,
    aPadBytesMultiple    = 110,
    aBlockByteLength     = 111,
    aNumberOfScanLines   = 115,
    aColorTreatment      = 120,
    aCommentData         = 129,
    aDataOrg             = 130,
    aMeasure             = 134,
    aSourceType          = 136,
    aUnitsPerMeasure     = 137,
    aStreamName          = 139,
    aStreamDataLength    = 140,
    aErrorReport         = 143,
    aCharAngle           = 161,
    aCharCode            = 162,
    aCharDataSize        = 163,
    aCharScale           = 164,
    aCharShear           = 165,
    aCharSize            = 166,
    aFontHeaderLength    = 167,
    aFontName            = 168,
    aFontFormat          = 169,
    aSymbolSet           = 170,
    aTextData            = 171,
    aCharSubModeArray    = 172,
    aXSpacingData        = 175,
    aYSpacingData        = 176,
    aCharBoldValue       = 177
};
                                          
enum Operator {
    oBeginSession        = 0x41,
    oEndSession          = 0x42,
    oBeginPage           = 0x43,
    oEndPage             = 0x44,
    oComment             = 0x47,
    oOpenDataSource      = 0x48,
    oCloseDataSource     = 0x49,
    oBeginFontHeader     = 0x4f,
    oReadFontHeader      = 0x50,
    oEndFontHeader       = 0x51,
    oBeginChar           = 0x52,
    oReadChar            = 0x53,
    oEndChar             = 0x54,
    oRemoveFont          = 0x55,
    oSetCharAttributes   = 0x56,
    oBeginStream         = 0x5b,
    oReadStream          = 0x5c,
    oEndStream           = 0x5d,
    oExecStream          = 0x5e,
    oRemoveStream        = 0x5f,
    oPopGS               = 0x60,
    oPushGS              = 0x61,
    oSetClipReplace      = 0x62,
    oSetBrushSource      = 0x63,
    oSetCharAngle        = 0x64,
    oSetCharScale        = 0x65,
    oSetCharShear        = 0x66,
    oSetClipIntersect    = 0x67,
    oSetClipRectangle    = 0x68,
    oSetClipToPage       = 0x69,
    oSetColorSpace       = 0x6a,
    oSetCursor           = 0x6b,
    oSetCursorRel        = 0x6c,
    oSetHalftoneMethod   = 0x6d,
    oSetFillMode         = 0x6e,
    oSetFont             = 0x6f,
    oSetLineDash         = 0x70,
    oSetLineCap          = 0x71,
    oSetLineJoin         = 0x72,
    oSetMiterLimit       = 0x73,
    oSetPageDefaultCTM   = 0x74,
    oSetPageOrigin       = 0x75,
    oSetPageRotation     = 0x76,
    oSetPageScale        = 0x77,
    oSetPatternTxMode    = 0x78,
    oSetPenSource        = 0x79,
    oSetPenWidth         = 0x7a,
    oSetROP              = 0x7b,
    oSetSourceTxMode     = 0x7c,
    oSetCharBoldValue    = 0x7d,
    oSetClipMode         = 0x7f,
    oSetPathToClip       = 0x80,
    oSetCharSubMode      = 0x81,
    oCloseSubPath        = 0x84,
    oNewPath             = 0x85,
    oPaintPath           = 0x86,
    oArcPath             = 0x91,
    oBezierPath          = 0x93,
    oBezierRelPath       = 0x95,
    oChord               = 0x96,
    oChordPath           = 0x97,
    oEllipse             = 0x98,
    oEllipsePath         = 0x99,
    oLinePath            = 0x9b,
    oLineRelPath         = 0x9d,
    oPie                 = 0x9e,
    oPiePath             = 0x9f,
    oRectangle           = 0xa0,
    oRectanglePath       = 0xa1,
    oRoundRectangle      = 0xa2,  
    oRoundRectanglePath  = 0xa3,
    oText                = 0xa8,
    oTextPath            = 0xa9,
    oBeginImage          = 0xb0,
    oReadImage           = 0xb1,
    oEndImage            = 0xb2,
    oBeginRastPattern    = 0xb3,
    oReadRastPattern     = 0xb4,
    oEndRastPattern      = 0xb5,
    oBeginScan           = 0xb6,
    oEndScan             = 0xb8,
    oScanLineRel         = 0xb9
};

enum MediaSize {
    eLetterPaper,
    eLegalPaper,
    eA4Paper,
    eExecPaper,
    eLedgerPaper,
    eA3Paper,
    eCOM10Envelope,
    eMonarchEnvelope,
    eC5Envelope,
    eDLEnvelope,
    eJB4Paper,
    eJB5Paper,
    etralala,
    eB5Envelope,
    eJPostcard,
    eJDoublePostcard,
    eA5Paper,
    eA6Paper,
    eJB6Paper
};

struct sPaperFormat {
    const char *name;
    int xl_nr;
    float width,height; /* inch */
} xlPaperFormats[] = {
    {"letter",eLetterPaper,8.5,11.0},
    {"legal",eLegalPaper,8.5,14},
    {"a4",eA4Paper,8.26389,11.6944},
    {"exec",eExecPaper,190/2.54,254/2.54},
    {"ledger",eLedgerPaper,279/2.54,432/2.54},
    {"a3",eA3Paper,11.6944,16.5278},
    {"com10envelope",eCOM10Envelope,105/2.54,241/2.54},
    {"monarchenvelope",eMonarchEnvelope,98/2.54,191/2.54},
    {"c5envelope",eC5Envelope,162/2.54,229/2.54},
    {"dlenvelope",eDLEnvelope,110/2.54,220/2.54},
    {"jb4",eJB4Paper,257/2.54,364/2.54},
    {"jb5",eJB5Paper,182/2.54,257/2.54},
    {"b5envelope",eB5Envelope,176/2.54,250/2.54},
    {"",-1,0,0},
    {"jpostcard",eJPostcard,148/2.54,200/2.54},
    {"jdoublepostcard",eJDoublePostcard,100/2.54,148/2.54},
    {"a5",eA5Paper, 5.84722,8.26389},
    {"a6",eA6Paper,4.125,5.84722},
    {"jb6",eJB6Paper,0,0},
    {NULL,0,0,0}
};

enum {
    eSTART,
    eRLE,
    eLIT 
} RLEstates;

#endif
