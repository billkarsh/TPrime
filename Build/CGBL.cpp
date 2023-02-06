
#include "CGBL.h"
#include "Cmdline.h"
#include "Util.h"

#include <QFileInfo>


/* --------------------------------------------------------------- */
/* Globals ------------------------------------------------------- */
/* --------------------------------------------------------------- */

CGBL    GBL;

/* --------------------------------------------------------------- */
/* PrintUsage  --------------------------------------------------- */
/* --------------------------------------------------------------- */

static void PrintUsage()
{
    Log();
    Log() << "*** ERROR: MISSING CRITICAL PARAMETERS ***\n";
    Log() << "------------------------";
    Log() << "Purpose:";
    Log() << "+ Map from-stream event times into to-stream times.";
    Log() << "Run messages are appended to TPrime.log in the current working directory.\n";
    Log() << "Usage:";
    Log() << ">TPrime -syncperiod=1.0 -tostream=path/edgefile.txt <from_data> [ options ]\n";
    Log() << "From_data:";
    Log() << "-fromstream=5,path/edgefile.txt                        ;stream_index,edge_times";
    Log() << "-events=5,path/in_eventfile.txt,path/out_eventfile.txt ;stream_index,in_event_times,out_event_times";
    Log() << "------------------------\n";
}

/* ---------------------------------------------------------------- */
/* CGBL ----------------------------------------------------------- */
/* ---------------------------------------------------------------- */

/* --------------------------------------------------------------- */
/* Public -------------------------------------------------------- */
/* --------------------------------------------------------------- */

bool CGBL::SetCmdLine( int argc, char* argv[] )
{
// Parse args

    const char  *sarg = 0;
    bool        ok;

    for( int i = 1; i < argc; ++i ) {

        if( GetArg( &period, "-syncperiod=%lf", argv[i] ) )
            ;
        else if( GetArgStr( sarg, "-tostream=", argv[i] ) )
            tostream.fedges = trim_adjust_slashes( sarg );
        else if( GetArgStr( sarg, "-fromstream=", argv[i] ) ) {

            const QStringList   sl = QString(sarg).split( ",", QString::SkipEmptyParts );

            if( sl.count() != 2 )
                goto bad_param;

            fromstream[sl.at(0).toInt( &ok )] =
                Stream( trim_adjust_slashes( sl.at(1) ) );

            if( !ok )
                goto bad_param;
        }
        else if( GetArgStr( sarg, "-events=", argv[i] ) ) {

            const QStringList   sl = QString(sarg).split( ",", QString::SkipEmptyParts );

            if( sl.count() != 3 )
                goto bad_param;

            events.push_back(
                Events( sl.at(0).toInt( &ok ),
                    trim_adjust_slashes( sl.at(1) ),
                    trim_adjust_slashes( sl.at(2) ) ) );

            if( !ok )
                goto bad_param;
        }
        else if( GetArgStr( sarg, "-offsets=", argv[i] ) )
            offsets = trim_adjust_slashes( sarg );
        else {
bad_param:
            Log() <<
            QString("Did not understand option '%1'").arg( argv[i] );
            return false;
        }
    }

// Check args

    if( !period || tostream.fedges.isEmpty()
        || fromstream.isEmpty() || events.isEmpty() ) {
        PrintUsage();
        return false;
    }

    if( !QFileInfo( tostream.fedges ).exists() ) {
        Log() << QString("To file does not exist: [%1]").arg( tostream.fedges );
        return false;
    }

    if( !offsets.isEmpty() && !QFileInfo( offsets ).exists() ) {
        Log() << QString("Offsets file does not exist: [%1]").arg( offsets );
        return false;
    }

// Echo

    QString sfrom       = "",
            sevents     = "",
            soffsets    = "";

    QMap<int,Stream>::iterator  it = fromstream.begin();

    for( ; it != fromstream.end(); ++it ) {
        Stream  &S = it.value();
        sfrom += QString(" -fromstream=%1,%2")
                    .arg( it.key() ).arg( S.fedges );
    }

    foreach( const Events &E, events ) {
        sevents += QString(" -events=%1,%2,%3")
                    .arg( E.istream ).arg( E.fin ).arg( E.fout );
    }

    if( !offsets.isEmpty() )
        soffsets = QString(" -offsets=%1").arg( offsets );

    Log() <<
        QString("Cmdline: TPrime -syncperiod=%1 -tostream=%2%3%4%5")
        .arg( period, 0, 'f', 6 )
        .arg( tostream.fedges )
        .arg( sfrom )
        .arg( sevents )
        .arg( soffsets );

    return true;
}

/* --------------------------------------------------------------- */
/* Private ------------------------------------------------------- */
/* --------------------------------------------------------------- */

QString CGBL::trim_adjust_slashes( const QString &dir )
{
    QString s = dir.trimmed();

    s.replace( "\\", "/" );
    return s.remove( QRegExp("/+$") );
}


