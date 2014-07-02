<?php

/**
 * @file
 * Definition of Drupal\system\Tests\Form\ElementsLabelsTest.
 */

namespace Drupal\system\Tests\Form;

use Drupal\simpletest\WebTestBase;

/**
 * Test form element labels, required markers and associated output.
 */
class ElementsLabelsTest extends WebTestBase {

  /**
   * Modules to enable.
   *
   * @var array
   */
  public static $modules = array('form_test');

  public static function getInfo() {
    return array(
      'name' => 'Form element and label output test',
      'description' => 'Test form element labels, required markers and associated output.',
      'group' => 'Form API',
    );
  }

  /**
   * Test form elements, labels, title attibutes and required marks output
   * correctly and have the correct label option class if needed.
   */
  function testFormLabels() {
    $this->drupalGet('form_test/form-labels');

    // Check that the checkbox/radio processing is not interfering with
    // basic placement.
    $elements = $this->xpath('//input[@id="edit-form-checkboxes-test-third-checkbox"]/following-sibling::label[@for="edit-form-checkboxes-test-third-checkbox" and @class="option"]');
    $this->assertTrue(isset($elements[0]), 'Label follows field and label option class correct for regular checkboxes.');

    // Make sure the label is rendered for checkboxes.
    $elements = $this->xpath('//input[@id="edit-form-checkboxes-test-0"]/following-sibling::label[@for="edit-form-checkboxes-test-0" and @class="option"]');
    $this->assertTrue(isset($elements[0]), 'Label 0 found checkbox.');

    $elements = $this->xpath('//input[@id="edit-form-radios-test-second-radio"]/following-sibling::label[@for="edit-form-radios-test-second-radio" and @class="option"]');
    $this->assertTrue(isset($elements[0]), 'Label follows field and label option class correct for regular radios.');

    // Make sure the label is rendered for radios.
    $elements = $this->xpath('//input[@id="edit-form-radios-test-0"]/following-sibling::label[@for="edit-form-radios-test-0" and @class="option"]');
    $this->assertTrue(isset($elements[0]), 'Label 0 found radios.');

    // Exercise various defaults for checkboxes and modifications to ensure
    // appropriate override and correct behavior.
    $elements = $this->xpath('//input[@id="edit-form-checkbox-test"]/following-sibling::label[@for="edit-form-checkbox-test" and @class="option"]');
    $this->assertTrue(isset($elements[0]), 'Label follows field and label option class correct for a checkbox by default.');

    // Exercise various defaults for textboxes and modifications to ensure
    // appropriate override and correct behavior.
    $elements = $this->xpath('//label[@for="edit-form-textfield-test-title-and-required" and @class="form-required"]/following-sibling::input[@id="edit-form-textfield-test-title-and-required"]');
    $this->assertTrue(isset($elements[0]), 'Label precedes textfield, with required marker inside label.');

    $elements = $this->xpath('//input[@id="edit-form-textfield-test-no-title-required"]/preceding-sibling::label[@for="edit-form-textfield-test-no-title-required" and @class="form-required"]');
    $this->assertTrue(isset($elements[0]), 'Label tag with required marker precedes required textfield with no title.');

    $elements = $this->xpath('//input[@id="edit-form-textfield-test-title-invisible"]/preceding-sibling::label[@for="edit-form-textfield-test-title-invisible" and @class="visually-hidden"]');
    $this->assertTrue(isset($elements[0]), 'Label preceding field and label class is visually-hidden.');

    $elements = $this->xpath('//input[@id="edit-form-textfield-test-title"]/preceding-sibling::span[@class="form-required"]');
    $this->assertFalse(isset($elements[0]), 'No required marker on non-required field.');

    $elements = $this->xpath('//input[@id="edit-form-textfield-test-title-after"]/following-sibling::label[@for="edit-form-textfield-test-title-after" and @class="option"]');
    $this->assertTrue(isset($elements[0]), 'Label after field and label option class correct for text field.');

    $elements = $this->xpath('//label[@for="edit-form-textfield-test-title-no-show"]');
    $this->assertFalse(isset($elements[0]), 'No label tag when title set not to display.');

    // Check #field_prefix and #field_suffix placement.
    $elements = $this->xpath('//span[@class="field-prefix"]/following-sibling::div[@id="edit-form-radios-test"]');
    $this->assertTrue(isset($elements[0]), 'Properly placed the #field_prefix element after the label and before the field.');

    $elements = $this->xpath('//span[@class="field-suffix"]/preceding-sibling::div[@id="edit-form-radios-test"]');
    $this->assertTrue(isset($elements[0]), 'Properly places the #field_suffix element immediately after the form field.');

    // Check #prefix and #suffix placement.
    $elements = $this->xpath('//div[@id="form-test-textfield-title-prefix"]/following-sibling::div[contains(@class, \'form-item-form-textfield-test-title\')]');
    $this->assertTrue(isset($elements[0]), 'Properly places the #prefix element before the form item.');

    $elements = $this->xpath('//div[@id="form-test-textfield-title-suffix"]/preceding-sibling::div[contains(@class, \'form-item-form-textfield-test-title\')]');
    $this->assertTrue(isset($elements[0]), 'Properly places the #suffix element before the form item.');

    // Check title attribute for radios and checkboxes.
    $elements = $this->xpath('//div[@id="edit-form-checkboxes-title-attribute"]');
    $this->assertEqual($elements[0]['title'], 'Checkboxes test' . ' (' . t('Required') . ')', 'Title attribute found.');
    $elements = $this->xpath('//div[@id="edit-form-radios-title-attribute"]');
    $this->assertEqual($elements[0]['title'], 'Radios test' . ' (' . t('Required') . ')', 'Title attribute found.');
  }
}
