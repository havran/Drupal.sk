<?php

/**
 * @file
 * Command-line tools to aid performing and developing migrations.
 */

use Drupal\Component\Utility\Unicode;
use Drupal\migrate\Entity\Migration;
use Drupal\migrate\Entity\MigrationInterface;
use Drupal\migrate_tools\MigrateExecutable;
use Drupal\migrate_tools\DrushLogMigrateMessage;
use Drupal\migrate_tools\MigrateManifest;
use Drupal\Core\Datetime\DateFormatter;

/**
 * Implements hook_drush_command().
 */
function migrate_tools_drush_command() {
  $items['migrate-status'] = array(
    'description' => 'List all migrations with current status.',
    'options' => array(
      'group' => 'Name of the migration group to list',
      'names-only' => 'Only return names, not all the details (faster)',
    ),
    'arguments' => array(
      'migration' => 'Restrict to a comma-separated list of migrations. Optional',
    ),
    'examples' => array(
      'migrate-status' => 'Retrieve status for all migrations',
      'migrate-status --group=beer' => 'Retrieve status for all migrations in a given group',
      'migrate-status BeerTerm,BeerNode' => 'Retrieve status for specific migrations',
    ),
    'drupal dependencies' => array('migrate_tools'),
    'aliases' => array('ms'),
  );

  $items['migrate-import'] = array(
    'description' => 'Perform one or more migration processes.',
    'options' => array(
      'all' => 'Process all migrations.',
      'group' => 'Name of the migration group to import',
      'limit' => 'Limit on the number of items to process in each migration',
      'feedback' => 'Frequency of progress messages, in items processed',
      'update' => ' In addition to processing unprocessed items from the source, update previously-imported items with the current data',
      'force' => 'Force an operation to run, even if all dependencies are not satisfied',
    ),
    'arguments' => array(
      'migration' => 'Name of migration(s) to import. Delimit multiple using commas.',
    ),
    'examples' => array(
      'migrate-import --all' => 'Perform all migrations',
      'migrate-import --group=beer' => 'Import all migrations in the beer group',
      'migrate-import BeerTerm,BeerNode' => 'Import new terms and nodes',
      'migrate-import BeerUser --limit=2' => 'Import no more than 2 users',
    ),
    'drupal dependencies' => array('migrate_tools'),
    'aliases' => array('mi'),
  );

  $items['migrate-stop'] = array(
    'description' => 'Stop an active migration operation.',
    'arguments' => array(
      'migration' => 'Name of migration to stop',
    ),
    'drupal dependencies' => array('migrate_tools'),
    'aliases' => array('mst'),
  );

  $items['migrate-manifest'] = array(
    'description' => 'Execute the migrations as specified in a manifest file.',
    'arguments' => array(
      'manifest' => 'The path to the manifest file',
    ),
    'required-arguments' => 1,
    'options' => array(
      'legacy-db-url' => array(
        'description' => 'A Drupal 6 style database URL.',
        'example-value' => 'mysql://root:pass@127.0.0.1/db',
        'required' => TRUE,
      ),
    ),
    'drupal dependencies' => array('migrate_tools'),
  );
  return $items;
}

/**
 * @param string $migration_names
 */
function drush_migrate_tools_migrate_status($migration_names = '') {
  $group_name = drush_get_option('group');
  $names_only = drush_get_option('names-only');

  $migrations = drush_migrate_tools_migration_list($group_name, $migration_names);

  $table = array();
  // Take it one group at a time, listing the migrations within each group.
  foreach ($migrations as $group_id => $migration_list) {
    if ($names_only) {
      $table[] = array(
        dt('Group: !name', array('!name' => $group_id))
      );
    }
    else {
      $table[] = array(
        dt('Group: !name', array('!name' => $group_id)),
        dt('Total'),
        dt('Imported'),
        dt('Unprocessed'),
        dt('Status'),
        dt('Last imported'),
      );
    }
    foreach ($migration_list as $migration_id => $migration) {
      try {
        $map = $migration->getIdMap();
        $imported = $map->importedCount();
        $source_plugin = $migration->getSourcePlugin();
      }
      catch (Exception $e) {
        continue;
      }
      try {
        $source_rows = $source_plugin->count();
        // -1 indicates uncountable sources.
        if ($source_rows == -1) {
          $source_rows = dt('N/A');
          $unprocessed = dt('N/A');
        }
        else {
          $unprocessed = $source_rows - $map->processedCount();
        }
      }
      catch (Exception $e) {
        drush_log(dt('Could not retrieve source count from !migration',
                      array('!migration' => $migration_id)));
        $source_rows = dt('N/A');
        $unprocessed = dt('N/A');
      }

      if ($names_only) {
        $table[] = array($migration_id);
      }
      else {
        $status = $migration->getStatus();
        switch ($status) {
          case MigrationInterface::STATUS_IDLE:
            $status = dt('Idle');
            break;
          case MigrationInterface::STATUS_IMPORTING:
            $status = dt('Importing');
            break;
          case MigrationInterface::STATUS_ROLLING_BACK:
            $status = dt('Rolling back');
            break;
          case MigrationInterface::STATUS_STOPPING:
            $status = dt('Stopping');
            break;
          default:
            $status = dt('Invalid');
            break;
        }
        $migrate_last_imported_store = \Drupal::keyValue('migrate_last_imported');
        $last_imported =  $migrate_last_imported_store->get($migration->id(), FALSE);
        if ($last_imported) {
          /** @var DateFormatter $date_formatter */
          $date_formatter = \Drupal::service('date.formatter');
          $last_imported = $date_formatter->format($last_imported / 1000,
            'custom', 'Y-m-d H:i:s');
        }
        else {
          $last_imported = '';
        }
        $table[] = array($migration_id, $source_rows, $imported, $unprocessed, $status, $last_imported);
      }
    }
  }
  drush_print_table($table);
}

/**
 * @param string $migration_names
 */
function drush_migrate_tools_migrate_import($migration_names = '') {
  $group_name = drush_get_option('group');
  $all = drush_get_option('all');
  $options = [];
  if (!$all && !$group_name && !$migration_names) {
    drush_set_error('MIGRATE_ERROR', dt('You must specify --all, --group, or one or more migration names separated by commas'));
    return;
  }

  if (drush_get_option('limit')) {
    $options['limit'] = drush_get_option('limit');
  }
  if (drush_get_option('feedback')) {
    $options['feedback'] = drush_get_option('feedback');
  }

  $update = drush_get_option('update');
  $force = drush_get_option('force');

  $log = new DrushLogMigrateMessage();

  $migrations = drush_migrate_tools_migration_list($group_name, $migration_names);

  // Take it one group at a time, importing the migrations within each group.
  foreach ($migrations as $group_id => $migration_list) {
    foreach ($migration_list as $migration_id => $migration) {
      if ($force) {
        $migration->set('requirements', []);
      }
      if ($update) {
        $migration->getIdMap()->prepareUpdate();
      }
      $executable = new MigrateExecutable($migration, $log, $options);
      // drush_op() provides --simulate support.
      drush_op(array($executable, 'import'));
    }
  }
}

/**
 * @param string $migration_id
 */
function drush_migrate_tools_migrate_stop($migration_id = '') {
  /** @var Migration $migration */
  $migration = Migration::load($migration_id);
  if ($migration) {
    $status = $migration->getStatus();
    switch ($status) {
      case MigrationInterface::STATUS_IDLE:
        drush_log(dt('Migration !id is idle', ['!id' => $migration_id]), 'warning');
        break;
      case MigrationInterface::STATUS_DISABLED:
        drush_log(dt('Migration !id is disabled', ['!id' => $migration_id]), 'warning');
        break;
      case MigrationInterface::STATUS_STOPPING:
        drush_log(dt('Migration !id is already stopping', ['!id' => $migration_id]), 'warning');
        break;
      default:
        $migration->interruptMigration(MigrationInterface::RESULT_STOPPED);
        drush_log(dt('Migration !id requested to stop', ['!id' => $migration_id]), 'success');
        break;
    }
  }
  else {
    drush_log(dt('Migration !id does not exist', ['!id' => $migration_id]), 'error');
  }
}

/**
 * Retrieve a list of active migrations.
 *
 * @param string $group_id
 *  Group machine name - if present, return only migrations in this group.
 * @param string $migration_ids
 *  Comma-separated list of migrations - if present, return only these migrations.
 *
 * @return MigrationInterface[][]
 *   An array keyed by migration group, each value containing an array of migrations.
 */
function drush_migrate_tools_migration_list($group_id = '', $migration_ids = '') {
  $query = \Drupal::entityQuery('migration');
  if ($group_id) {
    $query->condition('migration_group', $group_id);
  }
  $names = $query->execute();

  // Order the migrations according to their dependencies.
  /** @var MigrationInterface[] $migrations */
  $migrations = \Drupal::entityManager()
     ->getStorage('migration')
     ->loadMultiple($names);

  if (!empty($migration_ids)) {
    $migration_ids = explode(',', Unicode::strtolower($migration_ids));
  }
  else {
    $migration_ids = array();
  }

  $return = array();
  foreach ($migrations as $migration_id => $migration) {
    if (empty($migration_ids) || in_array(Unicode::strtolower($migration_id), $migration_ids)) {
      $group_id = $migration->get('migration_group');
      if (!empty($group_id)) {
        $return[$group_id][$migration_id] = $migration;
      }
      else {
        $return['default'][$migration_id] = $migration;
      }
    }
  }
  return $return;
}


/**
 * Import from a manifest file.
 *
 * This command allows you to specify a list of migrations and their config in
 * a YAML file. An example of a simple migration may look like this:
 *
 * @code
 *  - d6_action_settings
 *  - d6_aggregator_feed
 * @endcode
 *
 * You can also provide configuration to a migration for both source and the
 * destination. An example as such:
 *
 * @code
 *  - d6_file:
 *    source:
 *      conf_path: sites/assets
 *    destination:
 *      source_base_path: destination/base/path
 *      destination_path_property: uri
 *  - d6_action_settings
 * @endcode
 *
 * @param string $manifest
 *   The path to the manifest file.
 */
function drush_migrate_tools_migrate_manifest($manifest) {
  try {
    $manifest_runner = new MigrateManifest($manifest);
    return $manifest_runner->import();
  }
  catch (\Exception $e) {
    drush_set_error('MIGRATE_ERROR', $e->getMessage());
  }

  drush_invoke_process('@self', 'cache-rebuild', array(), array(), FALSE);
}
