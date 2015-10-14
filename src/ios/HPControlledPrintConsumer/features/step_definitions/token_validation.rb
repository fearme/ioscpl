Given /^I am on the Setup Screen$/ do
  element_exists ("label marked:'SquitchyTink Setup'")
  sleep(STEP_PAUSE)
end

Then /^I tap Print Money!$/ do
  element_exists ("label marked:'AVAILABLE PRINTERS'")
  sleep(STEP_PAUSE)
end

Then /^I do not get an error message$/ do
  if element_exists ("view marked:'Invalid Coupon'")
  	raise "Token Validation failed. The token should be valid."
  end
  sleep(STEP_PAUSE)
end

Given /^I go to Validation Settings$/ do
  touch ("label marked:'Validation Settings'")
  sleep(STEP_PAUSE)
end

Given /^I Create an invalid token$/ do
  touch("UITextField")
  clear_text ("UITextField")
  sleep(STEP_PAUSE)

  keyboard_enter_text("blah")
  sleep(STEP_PAUSE)
end

Then /^I go back from here$/ do
  touch ("label marked: 'Back'")
  sleep(STEP_PAUSE)
end

Then /^I get an error message$/ do
  element_exists ("view marked:'Invalid Coupon'")
  sleep(STEP_PAUSE)
end