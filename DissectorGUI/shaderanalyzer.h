#ifndef SHADERANALYZER_H
#define SHADERANALYZER_H
#include <QList>
#include <QString>
class ShaderDebugWindow;

namespace ShaderAnalyzer
{
    bool FindDiffs( const QList<QString>& iListing0,
                    const QList<QString>& iListing1,
                    QVector<QString>& oExecutedLines0,
                    QVector<QString>& oExecutedLines1 );

    void AnalyzeShaders( ShaderDebugWindow* iWin0, ShaderDebugWindow* iWin1 );
};

#endif // SHADERANALYZER_H
