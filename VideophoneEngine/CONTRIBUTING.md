# Contributing to VideophoneEngine

Welcome to the VideophoneEngine project! This guide will help you understand how to contribute effectively to our codebase.

## Table of Contents

- [Getting Started](#getting-started)
- [Development Environment](#development-environment)
- [Code Standards](#code-standards)
- [Submitting Changes](#submitting-changes)
- [Testing](#testing)
- [Documentation](#documentation)
- [Communication](#communication)

## Getting Started

### About the Project

The VideophoneEngine is a comprehensive C/C++ videophone communication engine developed by Sorenson Communications Inc. It supports multiple platforms (Windows, Linux, macOS, iOS, Android) and various videophone devices (VP2, Lumina) and applications.

### Prerequisites

- **C++ Compiler**: Support for C++11 standard or later
- **Build Tools**: CMake 3.10+, Meson (optional), Make
- **Dependencies**: Poco C++ Libraries, Boost 1.78.0+, OpenSSL, SQLite3
- **Platform Tools**: Platform-specific SDKs for target devices

### Repository Structure

```
VideophoneEngine/
├── cci/                    # Core Communication Interface
├── ConfMgr/               # Conference Manager
├── Platform/              # Platform abstraction layer
├── services/              # Service managers
├── contacts/              # Contact management
├── common/               # Shared utilities
├── Libraries/            # Third-party dependencies
├── Tests/                # Unit and integration tests
├── CMakeLists.txt        # CMake build configuration
└── meson.build          # Meson build configuration
```

## Development Environment

### Initial Setup

1. **Clone the repository** and navigate to the project directory
2. **Install dependencies** according to your platform
3. **Configure build system**:
   ```bash
   # CMake
   mkdir build && cd build
   cmake -DAPPLICATION=APP_NTOUCH_PC -DDEVICE=DEV_NTOUCH_PC ..
   make
   
   # Meson
   meson build
   ninja -C build
   ```

### Build Configuration

The build system supports multiple configurations via key variables:
- `APPLICATION`: Target application (APP_NTOUCH_PC, APP_NTOUCH_IOS, etc.)
- `DEVICE`: Target device (DEV_X86, DEV_NTOUCH_VP3, etc.)
- `MODE`: Build mode (release, debug)

## Code Standards

This section defines coding standards that ensure consistency, maintainability, and quality across all code contributions to the VideophoneEngine.

**Key Requirements:**
- All code must be compatible with C++11 standard or later
- Must support cross-platform compilation
- Must follow existing architectural patterns
- Must integrate with existing build systems (CMake, Meson, Make)

---

## General Principles

#### 1. Interface-Implementation Separation
- **Interface Classes**: Use `Isti*` prefix for pure virtual interfaces
- **Implementation Classes**: Use `Csti*` prefix for concrete implementations
- **Example**: `IstiBlockListManager` (interface) → `CstiBlockListManager` (implementation)

#### 2. Consistency First
**CRITICAL**: New code must match existing patterns in the same module. When in doubt, examine neighboring files and follow their conventions exactly.

---

## Naming Conventions

### Class Names
- **Interface Classes**: `Isti` prefix + descriptive name
  - Example: `IstiVideophoneEngine`, `IstiCall`, `IstiContactManager`
- **Implementation Classes**: `Csti` prefix + descriptive name
  - Example: `CstiConferenceManager`, `CstiBlockListManager`
- **Data Structures**: `Ssti` prefix for structs
  - Example: `SstiCallStatistics`, `SstiSharedContact`
- **Enums**: `Esti` prefix with descriptive name
  - Example: `EstiCallResult`, `EstiDialMethod`, `EstiInterfaceMode`

### Variable Names
- **Member Variables**: `m_` prefix with camelCase
  ```cpp
  bool m_bEnabled;
  CstiCoreService *m_pCoreService;
  unsigned int m_nMaxLength;
  ```
- **Parameter Variables**: Hungarian notation + camelCase
  ```cpp
  bool bEnabled              // boolean
  int nIndex                 // integer
  unsigned int unRequestId   // unsigned integer
  const char *pszName        // pointer to string (null-terminated)
  uint32_t un32Value         // 32-bit unsigned integer
  ```
- **Local Variables**: camelCase without prefix
  ```cpp
  std::string dialString;
  bool isInitialized;
  ```

### Function Names
- **Public Methods**: PascalCase with descriptive names
  ```cpp
  virtual stiHResult ItemAdd();
  virtual bool EnabledGet();
  virtual void EnabledSet(bool bEnabled);
  ```
- **Private Methods**: camelCase
  ```cpp
  void callbackMessageSend();
  stiHResult coreRequestSend();
  ```

### Constants and Defines
- **#define Constants**: ALL_CAPS with underscores
  ```cpp
  #define DEFAULT_MAX_BLOCK_LIST_LENGTH 10
  #define MAX_TRACE_MSG 10000
  ```
- **Enum Values**: `esti` prefix + descriptive name
  ```cpp
  enum EstiCallResult
  {
      estiCALL_SUCCESS = 0,
      estiCALL_FAILED,
      estiCALL_BUSY
  };
  ```

### File Names
- **Header Files**: Match class name exactly
  - `IstiBlockListManager.h`, `CstiBlockListManager.h`
- **Implementation Files**: Match header name with `.cpp` extension
  - `CstiBlockListManager.cpp`
- **Platform Files**: Include platform identifier
  - `CstiPlatform_Windows.cpp`, `CstiAudioInput_Linux.cpp`

---

## File Organization

### Directory Structure
Follow the existing modular structure:
```
VideophoneEngine/
├── cci/                    # Core Communication Interface
├── ConfMgr/               # Conference Manager
│   ├── DataTasks/         # Audio/Video/Text processing
│   ├── Nat/              # NAT traversal (ICE/STUN/TURN)
│   └── Session/          # SDP and media session handling
├── Platform/              # Platform abstraction layer
│   ├── common/           # Cross-platform code
│   ├── ntouch-pc/        # Windows/Linux desktop
│   ├── ntouch-apple/     # macOS/iOS
│   └── ntouch-android/   # Android
├── services/              # Service managers
├── contacts/              # Contact management
├── common/               # Shared utilities
└── Libraries/            # Third-party dependencies
```

### File Placement Rules
1. **Interface files** go in the root of their module directory
2. **Implementation files** go alongside their interfaces
3. **Platform-specific code** goes in appropriate Platform subdirectories
4. **Common utilities** go in the `common/` directory

---

## Header Files

### Header File Template
```cpp
/*!
 * \file ClassName.h
 * \brief Brief description of the class purpose
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright YYYY Sorenson Communications, Inc. -- All rights reserved
 */

#ifndef CLASSNAME_H
#define CLASSNAME_H

// System includes (alphabetical)
#include <memory>
#include <string>
#include <vector>

// Local includes (alphabetical)
#include "BaseClass.h"
#include "stiSVX.h"

// Forward declarations
class RelatedClass;

namespace WillowPM
{

/*!
 * \brief Brief class description
 * 
 * Detailed description of the class, its purpose,
 * and how it fits into the overall architecture.
 */
class ClassName : public BaseClass
{
public:
    // Constructors/Destructor
    ClassName();
    virtual ~ClassName();
    
    // Deleted copy/move operations (C++11 style)
    ClassName(const ClassName &other) = delete;
    ClassName(ClassName &&other) = delete;
    ClassName &operator=(const ClassName &other) = delete;
    ClassName &operator=(ClassName &&other) = delete;
    
    // Public interface methods
    virtual stiHResult MethodName() = 0;
    
private:
    // Private member variables
    bool m_bInitialized;
    std::string m_name;
};

} // namespace WillowPM

#endif // CLASSNAME_H
```

### Header Guidelines
1. **Always use header guards** (not `#pragma once` for maximum compatibility)
2. **Include order**: System headers first, then local headers, both alphabetical
3. **Forward declare** when possible to minimize dependencies
4. **Document all public methods** with Doxygen-style comments
5. **Use const correctness** throughout method declarations

---

## Implementation Files

### Implementation File Template
```cpp
/*!
 * \file ClassName.cpp
 * \brief Implementation of ClassName
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright YYYY Sorenson Communications, Inc. -- All rights reserved
 */

//
// Includes
//
#include "ClassName.h"
#include "stiTrace.h"
#include <sstream>

using namespace WillowPM;

//
// Constants
//
#define DEFAULT_TIMEOUT 5000

//
// Debug macros
//
#define DBG_MSG(fmt,...) stiDEBUG_TOOL(g_stiDebugFlag, stiTrace("Module: " fmt "\n", ##__VA_ARGS__);)

/*!
 * \brief Constructor
 *
 * Detailed description of what the constructor does,
 * any initialization performed, and preconditions.
 */
ClassName::ClassName()
:
    m_bInitialized(false),
    m_name("DefaultName")
{
    DBG_MSG("ClassName constructor called");
}

/*!
 * \brief Method implementation
 *
 * \param paramName Description of parameter
 * \return Description of return value
 */
stiHResult ClassName::MethodName()
{
    stiHResult hResult = stiRESULT_SUCCESS;
    
    // Implementation details here
    
    return hResult;
}
```

### Implementation Guidelines
1. **Match header structure** exactly
2. **Include necessary headers** in implementation file
3. **Use initialization lists** for constructors
4. **Initialize all member variables** in constructor
5. **Check preconditions** at method entry
6. **Use consistent error handling** patterns

---

## Code Style and Formatting

### Indentation and Spacing
- **Use tabs for indentation** (not spaces)
- **Align continuation lines** with opening parenthesis or use consistent indentation
- **Single space around operators**: `a + b`, `if (condition)`
- **No trailing whitespace**

### Braces and Control Structures
```cpp
// Function braces on new line
void functionName()
{
    // Implementation
}

// Control structure braces on new line
if (condition)
{
    // Code block
}
else
{
    // Alternative block
}

// Single-line conditions still use braces
if (simple)
{
    doSomething();
}
```

### Line Length
- **Maximum 120 characters per line**
- **Break long function calls** at logical points
- **Align parameters** for readability

### Pointer and Reference Declarations
```cpp
// Pointer asterisk attached to type
char *pszString;
const std::string *pName;

// Reference ampersand attached to type  
const MyClass &reference;
```

---

## Documentation Standards

### Doxygen Comment Style
Use the `/*!` style for all documentation comments:

```cpp
/*!
 * \brief Short description (one line)
 *
 * Longer description that can span multiple lines.
 * Explain the purpose, behavior, and any important details.
 *
 * \param paramName Description of the parameter
 * \param[in] inputParam Input-only parameter
 * \param[out] outputParam Output-only parameter  
 * \param[in,out] inoutParam Input/output parameter
 *
 * \return Description of return value
 * \retval SPECIFIC_VALUE When this specific value is returned
 *
 * \note Important notes about usage
 * \warning Warnings about potential issues
 * \see RelatedClass, RelatedMethod
 */
```

### Documentation Requirements
1. **All public methods** must be documented
2. **All classes** must have class-level documentation
3. **Complex algorithms** should have inline comments
4. **Parameter validation** should be documented
5. **Thread safety** should be noted when relevant

---

## Build System Integration

### CMakeLists.txt Structure
Follow existing CMake patterns:
```cmake
# Module-specific CMakeLists.txt
set(MODULE_NAME "ModuleName")

# Source files
set(MODULE_SOURCES
    ClassName.cpp
    AnotherClass.cpp
)

# Header files  
set(MODULE_HEADERS
    ClassName.h
    AnotherClass.h
)

# Create library
add_library(${MODULE_NAME} STATIC ${MODULE_SOURCES} ${MODULE_HEADERS})

# Dependencies
target_link_libraries(${MODULE_NAME} CommonLibrary)

# Platform-specific additions
if(WIN32)
    target_sources(${MODULE_NAME} PRIVATE WindowsSpecific.cpp)
endif()
```

### Meson Build Integration
```python
# meson.build file structure
module_sources = [
    'ClassName.cpp',
    'AnotherClass.cpp'
]

module_lib = static_library(
    'ModuleName',
    module_sources,
    dependencies: [common_dep],
    include_directories: include_directories('.')
)
```

### Configuration Integration
Use the `smiConfig.h` system for feature flags:
```cpp
#include "smiConfig.h"

#ifdef stiMESSAGE_MANAGER
// Message manager specific code
#endif

#if APPLICATION == APP_NTOUCH_PC
// PC-specific code
#elif APPLICATION == APP_NTOUCH_IOS
// iOS-specific code  
#endif
```

---

## Platform-Specific Guidelines

### Cross-Platform Considerations
1. **Use platform abstraction layer** for OS-specific functionality
2. **Avoid platform-specific includes** in common headers
3. **Use consistent integer types**: `uint32_t`, `int32_t`, etc.
4. **Handle endianness** in network code

### Platform-Specific File Organization
```cpp
// Common interface
class IstiAudioInput
{
    virtual stiHResult Start() = 0;
};

// Platform-specific implementations
Platform/
├── ntouch-pc/CstiAudioInput.h        // Windows/Linux
├── ntouch-apple/CstiAudioInput.h     // macOS/iOS  
└── ntouch-android/CstiAudioInput.h   // Android
```

---

## Error Handling

### Return Value Standards
Use the `stiHResult` type for all methods that can fail:

```cpp
// Success/failure methods
virtual stiHResult Initialize();
virtual stiHResult ProcessRequest();

// Query methods can return direct values
virtual bool IsEnabled() const;
virtual EstiState StateGet() const;
```

### Error Handling Patterns
```cpp
stiHResult MyClass::ProcessData()
{
    stiHResult hResult = stiRESULT_SUCCESS;
    
    // Validate parameters
    if (!m_bInitialized)
    {
        return stiRESULT_NOT_INITIALIZED;
    }
    
    // Perform operation
    hResult = SubOperation();
    if (hResult != stiRESULT_SUCCESS)
    {
        DBG_ERR_MSG("SubOperation failed: 0x%08x", hResult);
        return hResult;
    }
    
    return stiRESULT_SUCCESS;
}
```

---

## Memory Management

### RAII and Smart Pointers
```cpp
// Use smart pointers for automatic cleanup
std::shared_ptr<MyClass> instance = std::make_shared<MyClass>();
std::unique_ptr<Resource> resource = std::make_unique<Resource>();

// Traditional pointer management (when required by interfaces)
MyClass *pInstance = new MyClass();
// ... use pInstance
delete pInstance;
pInstance = nullptr;
```

### Memory Safety Rules
1. **Initialize all pointers** to `nullptr`
2. **Check for null** before dereferencing
3. **Match new/delete** and `malloc/free`
4. **Use RAII** for resource management
5. **Prefer smart pointers** for ownership

---

## Threading and Synchronization

### Thread Safety Patterns
```cpp
class ThreadSafeClass
{
private:
    mutable std::mutex m_mutex;
    
public:
    void Lock() const { m_mutex.lock(); }
    void Unlock() const { m_mutex.unlock(); }
    
    bool ThreadSafeMethod() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        // Thread-safe implementation
        return true;
    }
};
```

### Synchronization Guidelines
1. **Document thread safety** in class documentation
2. **Use RAII locking** (`std::lock_guard`) when possible
3. **Avoid recursive mutexes** unless absolutely necessary
4. **Consider atomic operations** for simple shared state

---

## Configuration Management

### Feature Configuration
Use the configuration system properly:

```cpp
// Feature availability check
#ifdef stiIMAGE_MANAGER
    virtual IstiImageManager *ImageManagerGet() = 0;
#endif

// Application-specific code
#if APPLICATION == APP_NTOUCH_PC || APPLICATION == APP_NTOUCH_MAC
    #define DEFAULT_CONTROL_RANGE 256
#else
    #define DEFAULT_CONTROL_RANGE 12
#endif

// Device-specific code  
#if DEVICE == DEV_NTOUCH_VP3
    // VP3-specific implementation
#endif
```

---

## Examples

### Complete Class Example

**IstiExampleManager.h:**
```cpp
/*!
 * \file IstiExampleManager.h
 * \brief Interface for example management functionality
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2025 Sorenson Communications, Inc. -- All rights reserved
 */

#ifndef ISTIEXAMPLEMANAGER_H
#define ISTIEXAMPLEMANAGER_H

#include <string>
#include "stiSVX.h"

/*!
 * \brief Manages example operations
 * 
 * This interface provides methods for managing example data,
 * including adding, removing, and querying example items.
 */
class IstiExampleManager
{
public:
    /*!
     * \brief Adds a new example item
     *
     * \param pszName The name of the example item
     * \param nValue The value associated with the item
     * \param punRequestId Optional request ID for tracking
     *
     * \return stiRESULT_SUCCESS on success, error code on failure
     */
    virtual stiHResult ItemAdd(
        const char *pszName,
        int nValue,
        unsigned int *punRequestId = nullptr) = 0;
    
    /*!
     * \brief Gets the enabled state
     *
     * \return true if enabled, false otherwise
     */
    virtual bool EnabledGet() const = 0;
    
protected:
    IstiExampleManager() = default;
    virtual ~IstiExampleManager() = default;
};

#endif // ISTIEXAMPLEMANAGER_H
```

**CstiExampleManager.h:**
```cpp
/*!
 * \file CstiExampleManager.h
 * \brief Implementation of example management functionality
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2025 Sorenson Communications, Inc. -- All rights reserved
 */

#ifndef CSTIEXAMPLEMANAGER_H
#define CSTIEXAMPLEMANAGER_H

#include "IstiExampleManager.h"
#include <list>
#include <mutex>

namespace WillowPM
{

/*!
 * \brief Concrete implementation of IstiExampleManager
 */
class CstiExampleManager : public IstiExampleManager
{
public:
    CstiExampleManager(bool bEnabled);
    ~CstiExampleManager() override = default;
    
    // Deleted copy/move operations
    CstiExampleManager(const CstiExampleManager &other) = delete;
    CstiExampleManager(CstiExampleManager &&other) = delete;
    CstiExampleManager &operator=(const CstiExampleManager &other) = delete;
    CstiExampleManager &operator=(CstiExampleManager &&other) = delete;
    
    // IstiExampleManager implementation
    stiHResult ItemAdd(
        const char *pszName,
        int nValue,
        unsigned int *punRequestId = nullptr) override;
    
    bool EnabledGet() const override;

private:
    bool m_bEnabled;
    mutable std::mutex m_mutex;
    std::list<std::string> m_items;
    
    void validateParameters(const char *pszName) const;
};

} // namespace WillowPM

#endif // CSTIEXAMPLEMANAGER_H
```

**CstiExampleManager.cpp:**
```cpp
/*!
 * \file CstiExampleManager.cpp
 * \brief Implementation of CstiExampleManager
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2025 Sorenson Communications, Inc. -- All rights reserved
 */

//
// Includes
//
#include "CstiExampleManager.h"
#include "stiTrace.h"
#include <stdexcept>

using namespace WillowPM;

//
// Constants
//
#define MAX_EXAMPLE_ITEMS 100

//
// Debug macros
//
#define DBG_MSG(fmt,...) stiDEBUG_TOOL(g_stiExampleDebug, stiTrace("ExampleMgr: " fmt "\n", ##__VA_ARGS__);)

/*!
 * \brief Constructor
 *
 * Initializes the example manager with the specified enabled state.
 *
 * \param bEnabled Initial enabled state
 */
CstiExampleManager::CstiExampleManager(bool bEnabled)
:
    m_bEnabled(bEnabled)
{
    DBG_MSG("CstiExampleManager created, enabled=%d", bEnabled);
}

/*!
 * \brief Adds a new example item
 *
 * \param pszName The name of the example item
 * \param nValue The value associated with the item
 * \param punRequestId Optional request ID for tracking
 *
 * \return stiRESULT_SUCCESS on success, error code on failure
 */
stiHResult CstiExampleManager::ItemAdd(
    const char *pszName,
    int nValue,
    unsigned int *punRequestId)
{
    stiHResult hResult = stiRESULT_SUCCESS;
    
    try
    {
        validateParameters(pszName);
        
        std::lock_guard<std::mutex> lock(m_mutex);
        
        if (m_items.size() >= MAX_EXAMPLE_ITEMS)
        {
            return stiRESULT_BUFFER_FULL;
        }
        
        m_items.push_back(std::string(pszName));
        
        if (punRequestId)
        {
            *punRequestId = static_cast<unsigned int>(m_items.size());
        }
        
        DBG_MSG("Added item '%s' with value %d", pszName, nValue);
    }
    catch (const std::exception &e)
    {
        DBG_ERR_MSG("Exception in ItemAdd: %s", e.what());
        hResult = stiRESULT_INVALID_PARAMETER;
    }
    
    return hResult;
}

/*!
 * \brief Gets the enabled state
 *
 * \return true if enabled, false otherwise
 */
bool CstiExampleManager::EnabledGet() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_bEnabled;
}

/*!
 * \brief Validates input parameters
 *
 * \param pszName Name parameter to validate
 */
void CstiExampleManager::validateParameters(const char *pszName) const
{
    if (!pszName || strlen(pszName) == 0)
    {
        throw std::invalid_argument("Name parameter cannot be null or empty");
    }
}
```

## Submitting Changes

### Before You Submit

1. **Follow existing patterns** in the same module
2. **Test your changes** on target platforms
3. **Update documentation** for public APIs
4. **Check build systems** (CMake, Meson, Make all work)
5. **Run existing tests** to ensure no regressions

### Commit Guidelines

- **Keep commits focused** on single logical changes
- **Write clear commit messages** describing what and why
- **Include relevant issue numbers** in commit messages
- **Test each commit** to ensure it builds and runs

### Pull Request Process

1. **Create feature branch** from appropriate base branch
2. **Make your changes** following code standards
3. **Test thoroughly** on target platforms
4. **Update documentation** as needed
5. **Submit pull request** with clear description
6. **Address review feedback** promptly

## Testing

### Test Structure
```
Tests/
├── Unit/               # Unit tests
├── Integration/        # Integration tests
└── Platform/          # Platform-specific tests
```

### Running Tests

```bash
# CMake build
cd build
make test

# Check for specific platforms
make test_ntouch_pc
make test_ntouch_ios
```

### Writing Tests

Follow existing test patterns and ensure:
- **Test edge cases** and error conditions
- **Mock external dependencies** when needed
- **Test platform-specific code** on target platforms
- **Document test requirements** and setup

## Documentation

### Code Documentation

Use Doxygen-style comments for all public APIs:

```cpp
/*!
 * \brief Short description (one line)
 *
 * Longer description explaining purpose, behavior,
 * and important implementation details.
 *
 * \param paramName Description of parameter
 * \param[in] inputParam Input-only parameter
 * \param[out] outputParam Output-only parameter
 * \param[in,out] inoutParam Input/output parameter
 *
 * \return Description of return value
 * \retval SPECIFIC_VALUE When this specific value is returned
 *
 * \note Important usage notes
 * \warning Potential issues or limitations
 * \see RelatedClass, RelatedMethod
 */
```

### Requirements
- **All public methods** must be documented
- **All classes** need class-level documentation  
- **Complex algorithms** should have inline comments
- **Thread safety** must be documented when relevant
- **Platform limitations** should be noted

## Communication

### Getting Help

- **Code Questions**: Review existing code in the same module for patterns
- **Architecture Questions**: Consult existing documentation
- **Build Issues**: Check CMakeLists.txt and meson.build files
- **Platform Issues**: Look in Platform/ directory for examples

### Best Practices

1. **Study existing code** before implementing new features
2. **Ask questions early** if you're unsure about approaches
3. **Share knowledge** through clear documentation and comments
4. **Follow the principle of least surprise** - make code predictable
5. **Consider all target platforms** when making changes

### Code Review Guidelines

When reviewing code:
- **Check consistency** with existing patterns
- **Verify platform compatibility** across all targets
- **Ensure proper error handling** and resource management
- **Look for potential threading issues** 
- **Validate documentation** completeness and accuracy

---

## Quick Reference

### Essential Files to Review
- `smiConfig.h` - Feature and platform configuration
- `stiSVX.h` - Common types and definitions
- Existing code in the same module you're working on

### Key Principles
1. **Consistency over personal preference**
2. **Platform compatibility first**
3. **Interface-implementation separation**
4. **Comprehensive documentation**
5. **Thorough testing**

### Common Patterns
- Use `stiHResult` for error-prone operations
- Initialize all member variables in constructors
- Use Hungarian notation for parameters
- Follow existing debug macro patterns
- Implement proper copy/move semantics (usually deleted)

---

Thank you for contributing to the VideophoneEngine project! Your adherence to these guidelines helps maintain code quality and ensures the engine continues to serve users across all supported platforms effectively.