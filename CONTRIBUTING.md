# Contributing to SignVideo iOS

Thank you for your interest in contributing to the SignVideo iOS application! This document outlines our coding standards, branching model, and contribution process.

## Table of Contents

- [Coding Standards](#coding-standards)
- [Branching Model](#branching-model)
- [Pull Request Process](#pull-request-process)
- [Release Process](#release-process)
- [Development Workflow](#development-workflow)

## Coding Standards

### Swift Code Style

#### File Headers
All Swift files should include the standard file header:
```swift
//
//  FileName.swift
//  SignVideo
//
//  Created by [Author Name] on [Date].
//  Copyright © [Year] Sorenson Communications. All rights reserved.
//
```

#### Class and Structure Naming
- Use **PascalCase** for class, structure, and protocol names
- Use **camelCase** for properties, methods, and variables
- Use descriptive names that clearly indicate purpose

```swift
// Good
class AnalyticsManager: NSObject {
    static let shared = AnalyticsManager()
    private var pendoAPIKey: String
    
    func trackUsage(_ event: AnalyticsEvent, properties: [String: Any]?) {
        // Implementation
    }
}

// Avoid
class AM {
    var k: String
}
```

#### Property Declaration
- Use `let` for constants, `var` for variables
- Include access modifiers (private, public, internal) explicitly when needed
- Group related properties together

```swift
class BuildVersion: NSObject {
    var major = 0
    var minor = 0 
    var revision = 0
    var buildNumber = 0
    var versionString: String
}
```

#### Method Formatting
- Use clear, descriptive method names
- Include parameter labels for clarity
- Keep methods focused on single responsibilities

```swift
func isVersionNewerCurrentVersion(_ currentVersion: String, updateVersion: String) -> Bool {
    let currentBuildVersion = BuildVersion(string: currentVersion)
    let updateBuildVersion = BuildVersion(string: updateVersion)
    return updateBuildVersion.isGreaterThanBuildVersion(currentBuildVersion)
}
```

#### Control Flow
- Use consistent bracket placement (opening brace on same line)
- Include spaces around operators
- Use early returns to reduce nesting

```swift
func reloadBlockeds() {
    guard var tempBlockeds = SCIBlockedManager.shared.blockeds as? Array<SCIBlocked> else {
        return
    }
    
    let searchText = searchController.searchBar.text!
    if searchText.count > 0 && tempBlockeds.count > 0 {
        let filteredContactsArray = tempBlockeds.filter { blocked in
            return blocked.title.lowercased().contains(searchText.lowercased()) || 
                   blocked.number.lowercased().contains(searchText.lowercased())
        }
        tempBlockeds = filteredContactsArray
    }
}
```

### Objective-C Code Style

#### File Headers
All Objective-C files should include the standard header:
```objective-c
//
//  FileName.h/.m
//  SignVideo
//
//  Sorenson Communications Inc. Confidential. --  Do not distribute
//  Copyright [Year] Sorenson Communications, Inc. -- All rights reserved
//
```

#### Naming Conventions
- Use **PascalCase** for class names
- Use **camelCase** for method and property names
- Prefix classes with appropriate prefixes (SCI for Sorenson Communications Inc.)
- Use descriptive method names with clear parameter labels

```objective-c
// Good
@interface SCIContactManager : NSObject
- (void)saveContactWithName:(NSString *)name phoneNumber:(NSString *)phoneNumber;
@end

// Avoid
@interface CM : NSObject
- (void)save:(NSString *)n phone:(NSString *)p;
@end
```

#### Method Implementation
- Include proper spacing around brackets and operators
- Use consistent indentation (tabs)
- Group related methods together

### Color and Asset Guidelines

#### Color Definitions
Use the established color palettes defined in `Theme.swift`:
- **MarketingColors** for marketing-related UI elements
- **VPColors** for videophone-specific UI components

```swift
// Use established colors
let primaryColor = MarketingColors.sorensonGold
let backgroundColor = VPColors.lightGray

// Avoid creating new arbitrary colors
let customColor = UIColor(red: 0.5, green: 0.5, blue: 0.5, alpha: 1.0) // Don't do this
```

#### Asset Naming
- Use descriptive names for assets
- Include resolution indicators (@2x, @3x)
- Group related assets with consistent prefixes

Examples from the codebase:
- `switch_camera.png`, `switch_camera@2x.png`, `switch_camera@3x.png`
- `CameraStarting.png`, `CameraStarting@2x.png`

### Code Organization

#### Import Statements
- Group imports logically (Foundation first, then UIKit, then third-party)
- Use `@objc` attributes when Swift classes need Objective-C exposure

```swift
import Foundation
import UIKit
import Pendo

@objc class AnalyticsManager: NSObject {
    // Implementation
}
```

#### Extensions
- Use extensions to separate functionality logically
- Place extensions in the same file as the main class when closely related

## Branching Model

We follow a **trunk-based development** model with the following requirements:

### 1. Jira Ticket Requirement
- **ALL development work must begin with a Jira ticket**
- Create the Jira ticket first, then create your branch
- Use the Jira ticket number to name your branch

### 2. Branch Naming Convention
Branch names and pull requests **MUST** use the Jira ticket number as a prefix:

```
Format: IRIS-[ticket number]: [Brief description]

Examples:
- IRIS-123: Fix camera switching bug
- IRIS-456: Add new user authentication
- IRIS-789: Update UI for accessibility compliance
```

### 3. Main Branch Strategy
- **All release builds come from the `main` branch**
- The `main` branch should always be in a releasable state
- Direct commits to `main` are not allowed - use pull requests

### 4. Feature Development
1. Create Jira ticket for your work
2. Create feature branch from `main` using naming convention
3. Develop your feature with regular commits
4. Create pull request when ready for review
5. Address review feedback
6. Merge to `main` after approval

### 5. Hot Fix Process
Hot fixes follow a special process:

1. Create Jira ticket for the critical issue
2. Create release branch from `main`: `release/IRIS-[ticket-number]`
3. Apply the minimal fix to the release branch
4. Create pull request from release branch to `main`
5. **Immediately merge the release branch back into `main`** after hot fix deployment
6. Tag the hot fix release appropriately

## Pull Request Process

### Requirements
- **All pull requests must be approved by at least one other developer**
- Pull request title must follow the Jira naming convention
- Include description of changes and any breaking changes
- Ensure all tests pass before requesting review
- Address all review feedback before merging

### Pull Request Template
```markdown
## IRIS-[Ticket Number]: [Brief Description]

### Description
Brief description of the changes made.

### Jira Ticket
Link to the Jira ticket: [IRIS-XXX](link-to-jira)

### Changes Made
- List specific changes
- Include any new dependencies
- Note any breaking changes

### Testing
- [ ] Unit tests pass
- [ ] UI tests pass (if applicable)  
- [ ] Manual testing completed

### Checklist
- [ ] Code follows established coding standards
- [ ] Documentation updated (if necessary)
- [ ] No sensitive information committed
- [ ] Appropriate reviewers assigned
```

### Code Review Guidelines
- Focus on code quality, maintainability, and adherence to standards
- Check for potential security issues
- Verify that changes align with the Jira requirements
- Ensure proper error handling and edge cases are covered

## Release Process

### Git Tagging
We use **Git tags to identify builds** and releases:

#### Tag Format
```
Format: [scheme]-[version]-[build-number]

Examples:
- SignVideo-iOS-XCBuild-1234
- SignVideo-iOS-Release-2.1.0
```

#### Automated Tagging
Our CI/CD pipeline automatically creates tags for builds using the format defined in `ci_scripts/ci_post_xcodebuild.sh`:

```bash
SAFE_SCHEME=${CI_XCODE_SCHEME}-XCBuild-${CI_BUILD_NUMBER}
git tag ${SAFE_SCHEME}
```

#### Manual Tagging
For release builds, create tags manually:
```bash
git tag -a SignVideo-iOS-Release-2.1.0 -m "Release version 2.1.0"
git push origin SignVideo-iOS-Release-2.1.0
```

### Release Preparation
1. Ensure all features for the release are merged to `main`
2. Update version numbers in appropriate files
3. Create release tag from `main`
4. Trigger release build from the tagged commit
5. Perform release testing
6. Deploy to App Store Connect

## Development Workflow

### Getting Started
1. Clone the repository
2. Install dependencies: `./setupFastlane.sh`
3. Copy user configuration: `./copyUserConfig.sh`
4. Open `ntouch.xcodeproj` in Xcode

### Running simulator builds
Open Terminal
```bash
cd dependencies/lib
cp ./simulatorbuild/*.a .
```
### Running Tests
```bash
# Run all tests
bundle exec fastlane test

# Run specific test suite
bundle exec fastlane test test_suite:"ChecksTests"

# Generate test report
bundle exec fastlane test_report
```

### Before Committing
1. Run tests to ensure no regressions
2. Verify code follows established standards
3. Check that no sensitive information is included
4. Ensure commit messages are descriptive

### Commit Message Guidelines
```
Format: IRIS-[ticket-number]: [Brief description]

Examples:
- IRIS-123: Fix camera initialization crash on iOS 15
- IRIS-456: Add accessibility labels to main navigation
- IRIS-789: Update third-party dependencies for security
```

## Questions or Issues?

If you have questions about these guidelines or need clarification on the development process:

1. Check existing documentation in the repository
2. Consult with the code owners listed in `CODEOWNERS`
3. Create a Jira ticket for process improvement suggestions

## Code Owners

Default reviewers for all code changes:
- @kselman  

---

*This document is maintained by the SignVideo iOS development team. Please keep it updated as our processes evolve.*