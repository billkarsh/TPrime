#ifndef TOOL_H
#define TOOL_H

#include <QFile>
#include <QTextStream>

#include <vector>

struct Stream;
struct Events;

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

#define BLOCKVALS   256

class InFileBase {
public:
    double  D[BLOCKVALS];
    bool    isEOF;
public:
    InFileBase() : isEOF(false) {}
    virtual ~InFileBase()       {}
    virtual bool open( const QString &path ) = 0;
    virtual int getData() = 0;
};

class InFileTxt : public InFileBase {
private:
    QFile   f;
public:
    virtual ~InFileTxt()    {f.close();}
    virtual bool open( const QString &path );
    virtual int getData();
};

class InFileNpy : public InFileBase {
private:
    FILE                *f;
    std::vector<size_t> shape;
    size_t              word_size;
    size_t              remVals;
public:
    InFileNpy() : f(0)      {}
    virtual ~InFileNpy()    {if( f ) fclose( f );}
    virtual bool open( const QString &path );
    virtual int getData();
private:
    void parseHdr();
};

class OutFileBase {
public:
    virtual ~OutFileBase()  {}
    virtual bool open( const QString &path ) = 0;
    virtual void putData( const double *D, int nd ) = 0;
};

class OutFileTxt : public OutFileBase {
private:
    QFile       f;
    QTextStream ts;
public:
    virtual ~OutFileTxt()   {f.close();}
    virtual bool open( const QString &path );
    virtual void putData( const double *D, int nd );
};

class OutFileNpy : public OutFileBase {
private:
    FILE    *f;
    quint64 n;
public:
    OutFileNpy() : f(0), n(0)   {}
    virtual ~OutFileNpy()       {if( f ) {writeHdr(); fclose( f );}}
    virtual bool open( const QString &path );
    virtual void putData( const double *D, int nd );
private:
    void writeHdr();
};

class Tool
{
private:

public:
    Tool();
    virtual ~Tool();

    void entrypoint();

private:
    bool loadEdges( Stream &S );
    bool loadOffsets( Stream &S );
    int typeAndIP( int &ip, const QString &name, QString *error );
    void doEvents( const Events &E );
};

#endif  // TOOL_H


