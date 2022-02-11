
#include "cnpy.h"
#include "Tool.h"
#include "CGBL.h"
#include "Util.h"

#include <QFileInfo>


/* ---------------------------------------------------------------- */
/* InFileTxt ------------------------------------------------------ */
/* ---------------------------------------------------------------- */

bool InFileTxt::open( const QString &path )
{
    f.setFileName( path );

    if( !f.open( QIODevice::ReadOnly | QIODevice::Text ) ) {
        Log() << QString("Error opening events input file: [%1]").arg( path );
        return false;
    }

    return true;
}


int InFileTxt::getData()
{
    if( isEOF )
        return 0;

    QRegExp re("\\s|,|;|:");
    int     N = 0;

    while( N < BLOCKVALS ) {

        double      t;
        QStringList sl;
        QString     line = f.readLine();
        bool        ok;

        if( line.isEmpty() ) {
exit:
            isEOF = true;
            break;
        }

        sl = line.split( re, QString::SkipEmptyParts );

        if( !sl.size() )
            goto exit;

        t = sl[0].toDouble( &ok );

        if( ok )
            D[N++] = t;
    }

    return N;
}

/* ---------------------------------------------------------------- */
/* InFileNpy ------------------------------------------------------ */
/* ---------------------------------------------------------------- */

bool InFileNpy::open( const QString &path )
{
    f = fopen( STR2CHR(path), "rb" );

    if( !f ) {
        Log() << QString("Error opening events input file: [%1]").arg( path );
        return false;
    }

    parseHdr();

    return true;
}


int InFileNpy::getData()
{
    if( isEOF )
        return 0;

    int N       = 0,
        nBytes  = word_size * qMin( remVals, size_t(BLOCKVALS) );

    if( !nBytes ) {
        isEOF = true;
        return 0;
    }

    size_t nb = fread( &D[0], 1, nBytes, f );

    N        = nb / word_size;
    remVals -= N;

    if( remVals <= 0 )
        isEOF = true;

    return N;
}


void InFileNpy::parseHdr()
{
    bool fortran_order;

    cnpy::parse_npy_header( f, word_size, shape, fortran_order );

    remVals = 1;
    for( int is = 0, ns = shape.size(); is < ns; ++is )
        remVals *= shape[is];
}

/* ---------------------------------------------------------------- */
/* OutFileTxt ----------------------------------------------------- */
/* ---------------------------------------------------------------- */

bool OutFileTxt::open( const QString &path )
{
    f.setFileName( path );
    ts.setDevice( &f );

    if( !f.open( QIODevice::WriteOnly | QIODevice::Text ) ) {
        Log() << QString("Error opening events output file: [%1]").arg( path );
        return false;
    }

    return true;
}


void OutFileTxt::putData( const double *D, int nd )
{
    for( int id = 0; id < nd; ++id )
        ts << QString("%1\n").arg( D[id], 0, 'f', 6 );
}

/* ---------------------------------------------------------------- */
/* OutFileNpy ----------------------------------------------------- */
/* ---------------------------------------------------------------- */

bool OutFileNpy::open( const QString &path )
{
    f = fopen( STR2CHR(path), "wb" );

    if( !f ) {
        Log() << QString("Error opening events output file: [%1]").arg( path );
        return false;
    }

    writeHdr();

    return true;
}


void OutFileNpy::putData( const double *D, int nd )
{
    fwrite( D, sizeof(double), nd, f );
    n += nd;
}


void OutFileNpy::writeHdr()
{
    std::vector<size_t> shape;

    shape.push_back( n );

    std::vector<char>   H = cnpy::create_npy_header<double>( shape );

    fseek( f, 0, SEEK_SET );
    fwrite( &H[0], 1, H.size(), f );
}

/* ---------------------------------------------------------------- */
/* Tool ----------------------------------------------------------- */
/* ---------------------------------------------------------------- */

Tool::Tool()
{
}


Tool::~Tool()
{
}


void Tool::entrypoint()
{
    if( !loadEdges( GBL.tostream ) )
        return;

    QMap<int,Stream>::iterator  it = GBL.fromstream.begin();
    for( ; it != GBL.fromstream.end(); ++it ) {
        if( !loadEdges( it.value() ) )
            return;
    }

    foreach( const Events &E, GBL.events )
        doEvents( E );
}


bool Tool::loadEdges( Stream &S )
{
    if( !QFileInfo( S.fedges ).exists() ) {
        Log() << QString("From file does not exist: [%1]").arg( S.fedges );
        return false;
    }

    QFile   f( S.fedges );
    if( !f.open( QIODevice::ReadOnly | QIODevice::Text ) ) {
        Log() << QString("Error opening file: [%1]").arg( S.fedges );
        return false;
    }

    double  t;
    QString line;
    while( !(line = f.readLine()).isEmpty() ) {

        bool ok;
        t = line.toDouble( &ok );
        if( ok )
            S.edges.push_back( t );
    }

    f.close();

    if( !S.edges.size() ) {
        Log() << QString("Edge file empty: [%1]").arg( S.fedges );
        return false;
    }

    return true;
}


void Tool::doEvents( const Events &E )
{
// ---------------
// Find fromstream
// ---------------

    QMap<int,Stream>::iterator  it = GBL.fromstream.find( E.istream );
    if( it == GBL.fromstream.end() ) {
        Log() << QString("Event has bad stream_index: [%1]").arg( E.istream );
        return;
    }

    const Stream    &FS = it.value();

// ---------------
// Open input file
// ---------------

    if( !QFileInfo( E.fin ).exists() ) {
        Log() << QString("Event file does not exist: [%1]").arg( E.fin );
        return;
    }

    InFileBase  *fin    = 0;
    OutFileBase *fout   = 0;
    QRegExp     reTxt(".txt$", Qt::CaseInsensitive),
                reNpy(".npy$", Qt::CaseInsensitive);

    if( E.fin.contains( reTxt ) )
        fin = new InFileTxt();
    else if( E.fin.contains( reNpy ) )
        fin = new InFileNpy();
    else {
        Log() << QString("Event file has unknown extension: [%1]").arg( E.fin );
        return;
    }

    if( !fin->open( E.fin ) )
        return;

// ----------------
// Open output file
// ----------------

    if( E.fout.contains( reTxt ) )
        fout = new OutFileTxt();
    else if( E.fout.contains( reNpy ) )
        fout = new OutFileNpy();
    else {
        Log() << QString("Event file has unknown extension: [%1]").arg( E.fout );
        goto close_in;
    }

    if( !fout->open( E.fout ) )
        goto close_in;

// --------------
// Process events
// --------------

    {   // process scope

        // ---------------
        // Init edge track
        // ---------------

        double  halfper = 0.5 * GBL.period;
        int     ito     = 0,
                ifm     = 0,
                nto     = GBL.tostream.edges.size(),
                nfm     = FS.edges.size(),
                nd;

        // -----------
        // Loop events
        // -----------

        while( (nd = fin->getData()) ) {

            for( int id = 0; id < nd; ++id ) {

                double  evt = fin->D[id],
                        eto = GBL.tostream.edges[ito],
                        efm = FS.edges[ifm];

                // Find nearest previous edges

                if( eto < evt ) {
                    while( ito + 1 < nto && GBL.tostream.edges[ito + 1] <= evt )
                        eto = GBL.tostream.edges[++ito];
                }

                if( efm < evt ) {
                    while( ifm + 1 < nfm && FS.edges[ifm + 1] <= evt )
                        efm = FS.edges[++ifm];
                }

                // Adjust eto to less than half-period of efm

                if( eto - efm > halfper ) {
                    do {
                        eto -= GBL.period;
                    } while( eto - efm > halfper );
                }
                else if( efm - eto > halfper ) {
                    do {
                        eto += GBL.period;
                    } while( efm - eto > halfper );
                }

                // Report relative to edge

                fin->D[id] = eto + (evt - efm);
            }

            fout->putData( fin->D, nd );
        }

    }   // process scope

// -----------
// Close files
// -----------

    delete fout;

close_in:
    delete fin;
}


