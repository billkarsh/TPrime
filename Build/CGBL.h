#ifndef CGBL_H
#define CGBL_H

#include <QMap>
#include <QVector>

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

struct Stream {
    QString         fedges;
    QVector<double> edges;
    Stream()                                            {}
    Stream( const QString &fedges ) : fedges(fedges)    {}
};

struct Events {
    int         istream;
    QString     fin,
                fout;
    Events()                                    {}
    Events( int istream, const QString &fin, const QString &fout )
    :   istream(istream), fin(fin), fout(fout)  {}
};

class CGBL
{
public:
    double              period;
    Stream              tostream;
    QMap<int,Stream>    fromstream;
    QVector<Events>     events;

public:
    CGBL() : period(0)  {}

    bool SetCmdLine( int argc, char* argv[] );
};

/* --------------------------------------------------------------- */
/* Globals ------------------------------------------------------- */
/* --------------------------------------------------------------- */

extern CGBL GBL;

#endif  // CGBL_H


