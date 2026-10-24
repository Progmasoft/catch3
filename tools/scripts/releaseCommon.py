import os
import re

from scriptCommon import catchPath

versionParser = re.compile( r'(\s*static\sVersion\sversion)\s*\(\s*(.*)\s*,\s*(.*)\s*,\s*(.*)\s*,\s*\"(.*)\"\s*,\s*(.*)\s*\).*' )
rootPath = os.path.join( catchPath, 'src/Progmasoft/Catch3' )
versionPath = os.path.join( rootPath, "EngineVersion.cpp" )

class Version:
    """The version of the Catch2 compatibility engine, from Progmasoft/Catch3/EngineVersion.cpp."""
    def __init__(self):
        f = open( versionPath, 'r' )
        for line in f:
            m = versionParser.match( line )
            if m:
                self.variableDecl = m.group(1)
                self.majorVersion = int(m.group(2))
                self.minorVersion = int(m.group(3))
                self.patchNumber = int(m.group(4))
                self.branchName = m.group(5)
                self.buildNumber = int(m.group(6))
        f.close()

    def getVersionString(self):
        versionString = '{0}.{1}.{2}'.format( self.majorVersion, self.minorVersion, self.patchNumber )
        if self.branchName != "":
            versionString = versionString + '-{0}.{1}'.format( self.branchName, self.buildNumber )
        return versionString
