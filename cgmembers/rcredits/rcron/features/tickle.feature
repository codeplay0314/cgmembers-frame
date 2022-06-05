Feature: Tickle
AS a member
I WANT to be reminded of things to do, when it's time,
SO I don't get forgotten and miss out on stuff.

Setup:
  Given members:
  | uid  | fullName | email | flags   | access    | floor | phone        | signed  |*
  | .ZZA | Abe One  | a@    |         | %today-1d |     0 | +14132530001 | 0       |
  | .ZZB | Bea Two  | b@    |         | %today-2d |     0 | +14132530002 | 0       |
  | .ZZD | Dee Four | d@    |         | %today-9d |     0 | +14132530004 | 0       |
  | .ZZE | Eve Five | e@    | ok      | %today-3m |     0 | +14132530005 | %now-5d |
  | .ZZF | Flo Six  | f@    | ok      | %today-3m |     0 | +14132530006 | %now-6d |

Scenario: A newbie has taken only the first step
  Given these "r_invites":
  | email | inviter | code   | invited    | invitee |*
  | d@    | .ZZE    | codeD1 | %today-11d | .ZZD    |
  And member ".ZZD" has done step "signup agree"
  When cron runs "tickle"
  Then we notice "do step one|sign in|daily messages" to member ".ZZD"
  And we notice "invitee slow" to member ".ZZE" with subs:
  | fullName | elapsed | step     |*
  | Dee Four |       9 | verifyid |
  
Scenario: A newbie has taken some steps but not all
  Given member ".ZZA" has done step "signup verifyid agree preferences verifyemail"
  When cron runs "tickle"
  Then we notice "take another step|sign in|daily messages" to member ".ZZA"

#Scenario: A newbie is on the verify step
#  Given member ".ZZA" has done step "sign contact donate proxies prefs photo connect"
#  And member ".ZZB" has done step "sign contact donate proxies prefs photo connect"
#  And member ".ZZD" has done step "sign contact donate proxies prefs photo connect"
#  When cron runs "tickle"
#  Then we notice "call bank|sign in" to member ".ZZB" with subs:
#  | when                       |*
#  | tomorrow (how about 10am?) |
#  And we notice "call bank|sign in" to member ".ZZD" with subs:
#  | when                      |*
#  | today between 9am and 4pm |
#  And we notice "gift sent" to member ".ZZA" with subs:
#  | amount | rewardAmount |*
#  |    $10 |        $0.50 |

Scenario: A nonmember has not accepted the invitation
  Given these "r_invites":
  | email           | inviter | code   | invited    |*
  | zot@example.com | .ZZF    | codeF1 | %today-21d |
  When cron runs "tickle"
  Then we email "nonmember" to member "zot@example.com" with subs:
  | fullName | code   | phone        | signed  | nudge         | noFrame |*
  | Flo Six  | codeF1 | 413-253-0006 | %mdY-6d | reminder last | 1       |
  And we notice "invite languishing" to member ".ZZF" with subs:
  | email           | elapsed |*
  | zot@example.com |      21 |

Scenario: A nonmember has not accepted the invitation from a not-yet-active member
  Given these "r_invites":
  | email           | inviter | code   | invited   |*
  | zot@example.com | .ZZA    | codeA1 | %today-8d |
  When cron runs "tickle"
  Then we do not email "zot@example.com"
#  Then we email "nonmember" to member "zot@example.com" with subs:
#  | inviterName | code   | site                      | nudge        | noFrame |*
#  | Abe One     | codeA1 | http://localhost/rMembers | reminder one | 1       |
  And we do not notice to member ".ZZA"

Scenario: A nonmember has accepted the invitation
  Given these "r_invites":
  | email           | inviter | code   | invited   | invitee |*
  | zot@example.com | .ZZA    | codeA1 | %today-8d | .ZZB    |
  When cron runs "tickle"
  Then we do not email "nonmember" to member "b@example.com"
  
Scenario: A nonmember has accepted an invitation from someone else instead
  Given these "r_invites":
  | email         | inviter | code   | invited   | invitee |*
  | b@example.com | .ZZA    | codeA1 | %today-8d | 0       |
  | b@example.com | .ZZD    | codeA1 | %today-5d | .ZZB    |
  When cron runs "tickle"
  Then we do not email "nonmember" to member "b@example.com"

Scenario: member has negative balance for several months
  Given members have:
  | uid  | balance | flags             | floor | wentNeg  | activated |*
  | .ZZA | -57.01  | ok,confirmed,debt | -500  | %now-16m | %now-60d  |
  When cron runs "tickle"
  Then members have:
  | uid  | floor                                                 |*
  | .ZZA | %(-500 * (1 - (1 / (%FLOOR_DECAY_MOS - (16-1))))) |
  # for example, if FLOOR_DECAY_MOS is 20, -500 would be multiplied by (1 - 1/5), resulting in a new floor of -400.
  
Skip this test doesn't work but the functionality gets tested elsewhere
Scenario: A member gets a credit line
# This fails if run on a day of the month that the previous month doesn't have (for example on 10/31)
  Given these "txs":
  | created   | amount | payer | payee | purpose |*
  | %today-1m |    300 | .ZZE | .ZZF | gift    |
  When cron runs "tickle"
  Then members have:
  | uid  | floor |*
  | .ZZE |   -50 |
Resume
#  And we notice "new floor|no floor effect" to member ".ZZE" with subs:
#  | limit |*
#  |  $50 |
# (feature temporarily disabled)

# only works if not the first of the month
#Scenario: A member gets no new credit line because it's the wrong day
#  Given these "txs2":
#  | payee | amount | created   |*
#  | .ZZE  | 500   | %today-6w |
#  And these "txs":
#  | created   | amount | payer | payee | purpose |*
#  | %today-5w |    300 | .ZZE | .ZZF | gift    |
#  When cron runs "tickle"
#  Then members have:
#  | uid  | floor |*
#  | .ZZE |     0 |

Skip (such a change doesn't get reported, but it still happens)
Scenario: A member gets no new credit line because the change would be minimal
  Given balances:
  | uid  | rewards |*
  | .ZZE | 500 |
  And members have:
  | uid  | floor |*
  | .ZZE |    49 |  
  And these "txs":
  | created   | amount | payer | payee | purpose |*
  | %today-5w |    300 | .ZZE | .ZZF | gift    |
  When cron runs "tickle"
  Then members have:
  | uid  | floor |*
  | .ZZE |    49 |
