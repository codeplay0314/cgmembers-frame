Feature: Connect a Bank Account
AS a member
I WANT to connect my bank account to my Common Good account
SO I can transfer funds easily to and/or fro

Setup:
  Given members:
  | uid  | fullName | minimum | floor | flags      | risks   | bankAccount |*
  | .ZZA | Abe One  |       0 |   -20 | ok,debt    |         |             |

Scenario: A member connects a bank account during signup
  When member ".ZZA" visits page "settings/fund"
  Then we show "Connect a Checking Account" with:
  | Routing |
  | Account |
  | Refills |

#  # plus these hidden fields that appear when user selects Connect Yes:
#  # (also the Save button changes to Connect)
#  | Routing: | | |
#  | Account: | | |
#  | Again:   | | |
#  | Refills: | No | Yes |
#  # plus these hidden fields that appear when user selects Refills Yes:
#  | Target Bal:   | | |
#  | Min Transfer: | | |
#  | Save Weekly:  | | |

  When member ".ZZA" completes form "settings/fund" with values:
  | op      | connect | routingNumber | bankAccount | bankAccount2 | refills | chox |*
  | Connect |       0 | %DEV_ROUTING  | 1234        | 1234         |       0 |    3 |
  And member ".ZZA" visits page "settings/fund"
  Then we show "Banking Settings" with:
  | Account: | xxxxxx1234 | |
  | Refills: | No | Yes |
  | Save     | | |
  
Scenario: A member connects a bank account later
  Given member ".ZZA" visits page "settings/fund"
  And step done "fund"
  When member ".ZZA" visits page "settings/fund"
  Then we show "Connect a Checking Account" with:
  | Connect:               | No | Yes |
  | Save                   | | |

#  # plus these hidden fields that appear when user selects Connect Yes:
#  # (also the Save button changes to Connect)
#  | Routing: | | |
#  | Account: | | |
#  | Again:   | | |
#  | Refills: | No | Yes |
#  # plus these hidden fields that appear when user selects Refills Yes:
#  | Target Bal:   | | |
#  | Min Transfer: | | |
#  | Save Weekly:  | | |

  When member ".ZZA" completes form "settings/fund" with values:
  | op      | connect | routingNumber | bankAccount | bankAccount2 | refills | chox |*
  | Connect |       1 | %DEV_ROUTING  | 1234        | 1234         |       0 |    2 |
  And member ".ZZA" visits page "settings/fund"
  Then we show "Banking Settings" with:
  | Account: | xxxxxx1234 | |
  | Refills: | No | Yes |
  | Save     | | |
  
Scenario: An active member connects a bank account with a bad routing number
  Given member ".ZZA" has "person" steps done: "signup agree fund"
  When member ".ZZA" visits page "settings/fund"
  Then we show "Connect a Checking Account" with:
  | Connect | No | Yes |

  When member ".ZZA" completes form "settings/fund" with values:
  | op      | connect | routingNumber | bankAccount | bankAccount2 | refills | chox |*
  | Connect |       1 | 987654321     | 1234        | 1234         |       0 |    2 |
  Then we say "error": "bad routing number"
  
Skip (not finished below here)
 
   | Connect | connect | routingNumber | bankAccount | bankAccount2 | refills | target | achMin | saveWeekly |*
