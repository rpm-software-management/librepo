Feature: Package download with fastestmirror and its cache enabled
  Check that download with fastestmirror and existing cache used
  is faster then download with fastestmirror without cache.

  Scenario: Fastest mirror enabled and cache doesn't exist
    Given metalinkurl: https://mirrors.fedoraproject.org/metalink?repo=fedora-20&arch=x86_64
    And Want to download location: Packages/l/librepo-1.2.1-2.fc20.x86_64.rpm
    When I download the package three times
    Then Third download with fastestmirrorcache should be the fastest

