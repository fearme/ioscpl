Feature: Token Validation
  I want to verify a valid token results in a printer list
  And verify that an invalid token results in a error message


@smoke
Scenario: Test Valid Token
  Given I am on the Setup Screen
  Then I tap Print Money!
  And I do not get an error message


@done
@smoke
Scenario: Test Invalid Token
  Given I am on the Setup Screen
  Then I go to Validation Settings
    And I Create an invalid token
  Then I go back from here
    And I tap Print Money!
  Then I get an error message




