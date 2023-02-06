
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


// Read up to BLOCKVALS values from file into D[].
// Return count.
//
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


// Read up to BLOCKVALS values from file into D[].
// Return count.
//
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
        Log() << QString("Edge file does not exist: [%1]").arg( S.fedges );
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

    return loadOffsets( S );
}


// Offset list from file:   [X, ..., 1E99], X < 0 if startsecs option.
// Offset list default:     [0, 1E99].
//
bool Tool::loadOffsets( Stream &S )
{
    if( !GBL.offsets.isEmpty() ) {

        QString msg;
        int     ip, type = typeAndIP( ip, S.fedges, &msg );

        if( type < 0 ) {
            Log() << msg;
            return false;
        }

        QFile   f( GBL.offsets );
        if( !f.open( QIODevice::ReadOnly | QIODevice::Text ) ) {
            Log() << QString("Error opening file: [%1]").arg( GBL.offsets );
            return false;
        }

        QString key, line;

        switch( type ) {
            case 0: key = QString("sec_imap%1:").arg( ip ); break;
            case 1: key = QString("sec_imlf%1:").arg( ip ); break;
            case 2: key = QString("sec_obx%1:").arg( ip ); break;
            case 3: key = "sec_nidq:"; break;
        }

        while( !(line = f.readLine()).isEmpty() ) {

            if( line.startsWith( key ) ) {

                const QStringList   sl = line.split(
                                            QRegExp("\\s+"),
                                            QString::SkipEmptyParts );
                int                 n  = sl.size();

                if( n < 2 || sl.at( 0 ) != key ) {
                    Log() << QString("Offsets key [%1] has bad format").arg( key );
                    f.close();
                    return false;
                }

                for( int i = 1; i < n; ++i )
                    S.off.push_back( sl.at( i ).toDouble() );

                break;
            }
        }

        f.close();

        if( !S.off.size() ) {
            Log() << QString("No offsets for file: [%1]").arg( S.fedges );
            return false;
        }
    }
    else
        S.off.push_back( 0 );

    S.off.push_back( 1E99 );
    return true;
}


// Return type code {-1=error, 0=AP, 1=LF, 2=OB, 3=NI}.
//
// ip = {-1=NI, 0+=substream}.
//
int Tool::typeAndIP( int &ip, const QString &name, QString *error )
{
    int type    = -1;
    ip          = -1;

    QString fname_no_path = QFileInfo( name ).fileName();
    QRegExp re_ap("\\.imec(\\d+)?\\.ap\\."),
            re_lf("\\.imec(\\d+)?\\.lf\\."),
            re_ob("\\.obx(\\d+)?\\.obx\\.");

    re_ap.setCaseSensitivity( Qt::CaseInsensitive );
    re_lf.setCaseSensitivity( Qt::CaseInsensitive );
    re_ob.setCaseSensitivity( Qt::CaseInsensitive );

    if( fname_no_path.contains( re_ap ) ) {
        type    = 0;
        ip      = re_ap.cap(1).toInt();
    }
    else if( fname_no_path.contains( re_lf ) ) {
        type    = 1;
        ip      = re_lf.cap(1).toInt();
    }
    else if( fname_no_path.contains( re_ob ) ) {
        type    = 2;
        ip      = re_ob.cap(1).toInt();
    }
    else if( fname_no_path.contains( ".nidq.", Qt::CaseInsensitive ) )
        type = 3;
    else if( error ) {
        *error =
            QString("Missing type key in file name [%1]")
            .arg( fname_no_path );
    }

    return type;
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
        Log() << QString("Event input file has unknown extension: [%1]").arg( E.fin );
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
        Log() << QString("Event output file has unknown extension: [%1]").arg( E.fout );
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

        double  halfper = 0.5 * GBL.period,
                to_llim = GBL.tostream.off[0],
                fm_llim = FS.off[0],
                fm_ulim = FS.off[1];
        int     ito_edg = 0,    // move edge up from here
                ifm_edg = 0,    // move edge up from here
                nto_edg = GBL.tostream.edges.size(),
                nfm_edg = FS.edges.size(),
                ioff    = 0,    // composing file containing evt
                nevt;

        // -----------
        // Loop events
        // -----------

        while( (nevt = fin->getData()) ) {

            for( int ievt = 0; ievt < nevt; ++ievt ) {

                double  evt = fin->D[ievt],
                        eto = GBL.tostream.edges[ito_edg],
                        efm = FS.edges[ifm_edg];

                // Which file contains event

                while( evt >= fm_ulim ) {
                    ++ioff;
                    fm_llim = fm_ulim;
                    to_llim = GBL.tostream.off[ioff];
                    fm_ulim = FS.off[ioff + 1];
                }

                // Find nearest previous edges

                if( eto < evt ) {
                    while( ito_edg + 1 < nto_edg && GBL.tostream.edges[ito_edg + 1] <= evt )
                        eto = GBL.tostream.edges[++ito_edg];
                }

                if( efm < evt ) {
                    while( ifm_edg + 1 < nfm_edg && FS.edges[ifm_edg + 1] <= evt )
                        efm = FS.edges[++ifm_edg];
                }

                // Evt and edges optimally from same composing file

                if( eto < to_llim ) {

                    // Find first edge in next file

                    int i_edg = ito_edg;

                    while( i_edg + 1 < nto_edg ) {
                        eto = GBL.tostream.edges[++i_edg];
                        if( eto >= to_llim )
                            break;
                    }

                    // Ensure edge is before evt

                    while( eto > evt )
                        eto -= GBL.period;
                }

                if( efm < fm_llim ) {

                    // Find first edge in next file

                    int i_edg = ifm_edg;

                    while( i_edg + 1 < nfm_edg ) {
                        efm = FS.edges[++i_edg];
                        if( efm >= fm_llim )
                            break;
                    }

                    // Ensure edge is before evt

                    while( efm > evt )
                        efm -= GBL.period;
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

                fin->D[ievt] = eto + (evt - efm);
            }

            fout->putData( fin->D, nevt );
        }

    }   // process scope

// -----------
// Close files
// -----------

    delete fout;

close_in:
    delete fin;
}


