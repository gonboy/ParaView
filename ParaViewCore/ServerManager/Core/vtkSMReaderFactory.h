/*=========================================================================

  Program:   ParaView
  Module:    vtkSMReaderFactory.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMReaderFactory - is a factory for creating a reader
// proxy based on the filename/extension.
// .SECTION Description
// vtkSMReaderFactory is a factory for creating a reader that reads a particular
// file. The reader factory needs to be configured to register the reader
// prototypes supported by the application. This is done automatically when
// the reader's proxy definition is registered AND if it has the extensions
// specified in the Hints section of the XML proxy definition. It is done
// with the following format:
// \verbatim
// <ReaderFactory extensions="[list of expected extensions]"
//     file_description="[description of the file]" />
// \endverbatim
//
// Once the factory has been configured, the API to create readers, get
// available readers etc. can be used.

#ifndef vtkSMReaderFactory_h
#define vtkSMReaderFactory_h

#include "vtkPVServerManagerCoreModule.h" //needed for exports
#include "vtkSMObject.h"

class vtkStringList;
class vtkPVXMLElement;
class vtkSMProxy;
class vtkSMSession;

class VTKPVSERVERMANAGERCORE_EXPORT vtkSMReaderFactory : public vtkSMObject
{
public:
  static vtkSMReaderFactory* New();
  vtkTypeMacro(vtkSMReaderFactory, vtkSMObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Cleanup all registered prototypes.
  void Initialize();

  // Description:
  // Register a prototype. If it is already registered this becomes
  // a no-op.
  void RegisterPrototype(const char* xmlgroup, const char* xmlname);

  // Description:
  // Returns true if a reader can be determined that can read the file.
  // When this returns true, one can use GetReaderGroup() and GetReaderName() to
  // obtain the xmlgroup and xmlname for the reader that can read the file.
  // When this returns false, use GetPossibleReaders() to get the list of
  // readers that can possibly read the file.
  bool CanReadFile(const char* filename, vtkSMSession* session);

  // Description:
  // Returns the xml-name for the reader that can read the file queried by the
  // most recent CanReadFile() call. This is valid only if CanReadFile()
  // returned true.
  vtkGetStringMacro(ReaderName);

  // Description:
  // Returns the xml-group for the reader that can read the file queried by the
  // most recent CanReadFile() call. This is valid only if CanReadFile()
  // returned true.
  vtkGetStringMacro(ReaderGroup);

  // Description:
  // Get the list of readers that can possibly read the file. This is used when
  // the factory cannot determine which reader to use for reading the file. The
  // user can then choose from the provided options.
  // Returns a list of 3-tuples where the 1st string is the group, the 2nd
  // string is the reader name and the 3rd string is the reader description
  // Note that the extension test is skipped in this case.
  vtkStringList* GetPossibleReaders(const char* filename, vtkSMSession* session);

  // Description:
  // Returns a list of 3-tuples where the 1st string is the group, the 2nd
  // string is the reader name and the 3rd string is the reader description
  // This returns all the possible readers with a given connection id.
  vtkStringList* GetReaders(vtkSMSession* session);

  // Description:
  // Returls list of readers that can read the file using its extension and
  // connection id.
  // Returns a list of 3-tuples where the 1st string is the group, the 2nd
  // string is the reader name and the 3rd string is the reader description
  vtkStringList* GetReaders(const char* filename, vtkSMSession* session);

  // Description:
  // Helper method to test is a file is readable on the server side. This has
  // nothing to do with the whether the file is readable by a reader, just test
  // the file permissions etc. Internally uses the ServerFileListing proxy.
  static bool TestFileReadability(const char* filename, vtkSMSession* session);

  // Description:
  // Returns a formatted string with all supported file types.
  // \c cid is not used currently.
  // An example returned string would look like:
  // \verbatim
  // "Supported Files (*.vtk *.pvd);;PVD Files (*.pvd);;VTK Files (*.vtk)"
  // \endverbatim
  const char* GetSupportedFileTypes(vtkSMSession* session);

  // Description:
  // Helper method to check if the reader can read the given file. This is a
  // generic method that simply tries to call CanReadFile() on the reader. If
  // the reader des not support CanReadFile() then we assume the reader can read
  // the file, and return true.
  static bool CanReadFile(const char* filename, vtkSMProxy* reader);
  static bool CanReadFile(const char* filename, const char* readerxmlgroup,
    const char* readerxmlname, vtkSMSession* session);

  // Description:
  // Returns the number of registered prototypes.
  unsigned int GetNumberOfRegisteredPrototypes();

  // Description:
  // Every time a new proxy definition is added we check to see if it is
  // a reader and then we add it to the list of available readers.
  void UpdateAvailableReaders();

  // Description:
  // Add/remove a group name to look for readers in. By default "source" is included.
  void AddGroup(const char* groupName);
  void RemoveGroup(const char* groupName);
  void GetGroups(vtkStringList* groups);

  // Description:
  // This function is for ParaView based applications that only wish to expose
  // a subset of the readers.  If this function is never called, the reader
  // factory will expose all the readers as it has in the past.  However, if
  // any readers are specified by passing their group name and reader name to
  // this function, then only those readers will be available in any reader
  // factories created by the application.  This is intended to be called at
  // the begining of the application's execution before any sessions are
  // created.
  static void AddReaderToWhitelist(const char* readerxmlgroup, const char* readerxmlname);

protected:
  vtkSMReaderFactory();
  ~vtkSMReaderFactory();

  // To support legacy configuration files.
  void RegisterPrototype(
    const char* xmlgroup, const char* xmlname,
    const char* extensions,
    const char* description);

  vtkSetStringMacro(ReaderName);
  vtkSetStringMacro(ReaderGroup);

  char* ReaderName;
  char* ReaderGroup;
  vtkStringList* Readers;

private:
  vtkSMReaderFactory(const vtkSMReaderFactory&) VTK_DELETE_FUNCTION;
  void operator=(const vtkSMReaderFactory&) VTK_DELETE_FUNCTION;

  class vtkInternals;
  vtkInternals* Internals;

};

#endif

