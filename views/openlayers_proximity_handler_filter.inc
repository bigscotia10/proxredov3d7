<?php


class openlayers_proximity_handler_filter extends views_handler_filter_numeric {

  // exposed filter options
  var $always_multiple = TRUE;

  var $entityInfo = '';
  var $entityName = '';
  var $hasArguments = false;

  function init(&$view, &$options) {
    parent::init($view, $options);
    $entities = entity_get_info();
    foreach ($entities as $entityName => $entityInfo)
    {
        if ($entityInfo['base table'] == $this->view->base_table)
        {
            $this->entityInfo = $entityInfo;
            $this->entityName = $entityName;
            break;
        }
    }
    if (isset($this->view->display[$this->view->current_display]->handler->options['path']))
    {
        $this->hasArguments = true;
    }
  }

  function op_between($field) {
    $this->op_process('between', $field);
  }

  function op_simple($field) {
    $this->op_process('simple', $field);
  }

  function op_process($op = 'simple', $field) {

    $this->secure_input();
    if ($this->options['location_as'] == 'entity') {
      $this->process_entity_proximity();
    }
    if ($this->options['location_as'] == 'location') {
      $this->process_location_proximity();
    }
    if ($this->options['location_as'] == 'argument') {
      $this->process_argument_proximity();
    }
  }

  /**
   * Display the filter on the administrative summary
   */
  function admin_summary() {
    if (!empty($this->options['exposed'])) {
      return t('exposed');
    }
    $options = $this->operator_options('short');
    $output = check_plain($options[$this->operator]);
    if (in_array($this->operator, $this->operator_values(2))) {
      $output .= ' ' . t('@min and @max @unit from @location', array('@min' => $this->value['min'], '@max' => $this->value['max'], '@unit' => $this->value['unit'], '@location' => $this->value['location']));
    }
    elseif (in_array($this->operator, $this->operator_values(1))) {
      $output .= ' ' . t('@value @unit from @location', array('@value' => $this->value['value'], '@unit' => $this->value['unit'], '@location' => $this->value['location']));
    }
    return $output;
  }

  /**
   * Information about options for all kinds of purposes will be held here.
   */
  function option_definition() {
    $options = parent::option_definition();
    $options['operator'] = array('default' => '<');
    $options['location_as'] = array('default' => 'location');

    $options['value']['contains']['unit'] = array('default' => variable_get('openlayers_proximity_unit', OPENLAYERS_PROXIMITY_DEFAULT_UNIT));
    $options['value']['contains']['location'] = array('default' => '');
    $options['value']['contains']['entity'] = array('default' => '');
    $options['value']['contains']['argument'] = array('default' => '');

    $options['expose']['contains']['use_unit'] = array('default' => FALSE);
    $options['expose']['contains']['unit'] = array('default' => 'unit');
    $options['expose']['contains']['use_location'] = array('default' => FALSE);
    $options['expose']['contains']['location'] = array('default' => 'location');
    $options['expose']['contains']['use_entity'] = array('default' => FALSE);
    $options['expose']['contains']['entity'] = array('default' => 'entity');
    $options['expose']['contains']['use_argument'] = array('default' => FALSE);
    $options['expose']['contains']['argument'] = array('default' => 'argument');

    return $options;
  }

  /**
   * Provide default options for exposed filters.
   */
  function expose_options() {
    parent::expose_options();
    $this->options['expose']['use_unit'] = FALSE;
    $this->options['expose']['unit'] = 'unit';
    $this->options['expose']['use_location'] = FALSE;
    $this->options['expose']['location'] = 'location';
    $this->options['expose']['use_entity'] = FALSE;
    $this->options['expose']['entity'] = 'entity';
    $this->options['expose']['use_argument'] = FALSE;
    $this->options['expose']['argument'] = 'argument';
  }

  /**
   * Handle the 'left' side of the exposed options form.
   */
  function expose_form(&$form, &$form_state) {
    parent::expose_form($form, $form_state);
    $this->expose_option_form($form, 'unit', 'unit of measurement');
    $this->expose_option_form($form, 'location', 'location');
    $this->expose_option_form($form, 'entity', $this->entityInfo['label']);
    if ($this->hasArguments)
    {
        $this->expose_option_form($form, 'argument', 'argument');
    }
  }

  /**
   * Validate the options form.
   */
  function expose_validate($form, &$form_state) {
    parent::expose_validate($form, $form_state);
    // @todo: validation.
  }

  /**
   * Provide a simple textfield for equality
   */
  function value_form(&$form, &$form_state) {
    parent::value_form($form, $form_state);

    $form['value']['location'] = array(
      '#title' => t('Location'),
      '#type' => 'textfield',
      '#size' => 20,
      '#default_value' => $this->options['value']['location'],
      '#description' => t('Location where to start to search from.'),
      '#dependency' => array(
        'radio:options[location_as]' => array('location'),
      ),
    );
    $form['value']['entity'] = array(
      '#title' => $this->entityInfo['label'],
      '#type' => 'select',
      '#default_value' => $this->options['value']['entity'],
      '#options' => $this->get_entity_options(),
      '#description' => t('@entity where to start to search from.', array('@entity' => $this->entityInfo['label'])),
      '#dependency' => array(
        'radio:options[location_as]' => array('entity'),
      ),
    );
    $form['value']['argument'] = array(
      '#title' => t('Argument'),
      '#type' => 'select',
      '#default_value' => '',
      '#options' => $this->get_argument_options(),
      '#description' => t('Argument in path containing @entity id where to start to search from.', array('@entity' => $this->entityInfo['label'])),
      '#dependency' => array(
        'radio:options[location_as]' => array('argument'),
      ),
    );
    $form['value']['unit'] = array(
      '#title' => t('Unit of measurement'),
      '#type' => 'select',
      '#options' => openlayers_proximity_get_available_units_for_select(),
      '#default_value' => $this->options['value']['unit'],
      '#description' => t('Select unit of measurement.'),
      '#prefix' => '<div class="views-left-30">',
      '#suffix' => '</div>',
    );
    $location_as_options = array(
        'location' => t('Get location from string'),
        'entity' => t('Get location from @entity', array('@entity' => $this->entityInfo['label'])),
    );
    if ($this->hasArguments)
    {
        $location_as_options['argument'] = t('Get location from argument in path');
    }
    $form['location_as'] = array(
      '#type' => 'radios',
      '#title' => t('Location'),
      '#default_value' => $this->options['location_as'],
      '#options' => $location_as_options,
    );
  }

  /**
   * Provide a form for setting the operator.
   */
  function operator_form(&$form, &$form_state) {
    parent::operator_form($form, $form_state);
    // At the end of the day proximity search is a rough estimation.
    $this->unset_attributes($form['operator']['#options'], array('<=', '=', '!=', '>='));
  }

  /**
   * Render our chunk of the exposed filter form when selecting
   */
  function exposed_form(&$form, &$form_state) {

    if (empty($this->options['exposed'])) {
      return;
    }

    $force_operator = FALSE;
    $operators = $this->operator_options();
    if (empty($this->options['expose']['use_operator'])) {
      $this->options['expose']['use_operator'] = TRUE;
      $force_operator = TRUE;
    }

    parent::exposed_form($form, $form_state);

    if ($force_operator) {
      $operator = $this->options['expose']['operator'];
      $form[$operator]['#type'] = 'hidden';
      $op = $form[$operator]['#value'] = $form[$operator]['#default_value'];
      if (strstr($op, 'between')) {
        unset($form[$this->options['expose']['identifier']]['value']);
      }
      else {
        unset($form[$this->options['expose']['identifier']]['min']);
        unset($form[$this->options['expose']['identifier']]['max']);
      }
      $this->unset_attributes($form[$operator], array('#options'));
    }

    // When exposed pull location and unit out of value form item.
    if (!empty($this->options['expose']['identifier'])) {

      // Unset useless form elements.
      $filter = &$form[$this->options['expose']['identifier']];

      // Strip dependencies off on exposed form.
      $key = $this->options['expose']['identifier'];
      foreach (element_children($form[$key]) as $el) {
        if (!empty($form[$key][$el]['#dependency']) && $form[$key][$el]['#title'] == 'Location') {
          $form[$key][$el]['#dependency'] = array();
        }
      }

      $this->unset_attributes($form, array('location_as'));

      // Turn exposed form into a fieldset to have better control over its rendering.
      $filter['#type'] = 'fieldset';
      $filter['#attributes'] = array('class' => array("openlayers-proximity-filter-exposed"));

      // Expose unit of measurement form, if necessary.
      if ($this->filter_item_is_exposed('unit')) {
        $name = $this->rename_exposed_filter_item($form, 'unit');
        $this->clean_exposed_filter_item($filter[$name]);
      }
      else {
        $this->unset_attributes($filter, array('unit'));
      }

      // Expose location form, if necessary.
      $name = $this->options['location_as'];
      if ($this->filter_item_is_exposed($name)) {

        $name = $this->rename_exposed_filter_item($form, $name);
        $this->clean_exposed_filter_item($filter[$name]);

        $filter[$name]['#weight'] = 100;
        $filter[$name]['#field_prefix'] =  t('from');
      }
      else {
        $this->unset_attributes($filter, array($name));
        $filter[$name]['#value'] = $this->filter_item_is_exposed('unit') ? '<div class=" form-item">' . $this->get_suffix() . '</div>' : '';
      }

      if ($this->options['location_as'] == 'location') {
        $this->unset_attributes($filter, array('entity'));
        $this->unset_attributes($filter, array('argument'));
      }
      if ($this->options['location_as'] == 'entity') {
        $this->unset_attributes($filter, array('location'));
        $this->unset_attributes($filter, array('argument'));
      }
      if ($this->options['location_as'] == 'argument') {
        $this->unset_attributes($filter, array('location'));
        $this->unset_attributes($filter, array('entity'));
      }

      // Add meaningful suffix to value.
      if (isset($filter['value'])) {

        $filter['value']['#size'] = 3;
        $filter['value']['#field_prefix'] = $force_operator ? $operators[$this->options['operator']] : '';
        $filter['value']['#field_suffix'] = $this->get_suffix();

        $this->unset_attributes($filter['value'], array('#default_value'));
      }

      // Add meaningful prefix/suffix to min max.
      if (isset($filter['min']) && isset($filter['max'])) {

        $filter['min']['#size'] = 3;
        $filter['min']['#field_prefix'] = $force_operator ? $operators[$this->options['operator']] : '';

        $filter['max']['#size'] = 3;
        $filter['max']['#field_prefix'] = t('and');
        $filter['max']['#field_suffix'] = $this->get_suffix();

        $this->unset_attributes($filter['max'], array('#title', '#default_value'));
        $this->unset_attributes($filter['min'], array('#default_value'));
      }
    }
  }


  /**
   * Validate the exposed filter form
   */
  function exposed_validate(&$form, &$form_state) {
    parent::exposed_validate($form, $form_state);
    // Sanitize "value", "min" and "max"
    $operators = $this->operators();
    $identifier = $this->options['expose']['identifier'];
    $location = $this->options['expose']['location'];
    if (isset($form[$identifier]['value'])) {
      $valuefields = array('value');
    }
    else {
      $valuefields = array('min', 'max');
    }
    foreach ($valuefields as $field) {
      $form_state['values'][$identifier][$field] = str_replace(',', '.', $form_state['values'][$identifier][$field]);
    }
    $values = $form_state['values'];
    $op = $values[$this->options['expose']['operator']];

    // Validate "value", "min" and "max"
    $fields = ($operators[$op]['method'] == 'op_between') ? array('min', 'max') : array('value');
    foreach ($fields as $key => $field) {
      if ($values[$identifier][$field]) {
        if (!is_numeric($values[$identifier][$field])) {
          form_set_error($identifier . '][' . $field, t('Please, provide a valid numeric value.'));
        }
        elseif ($values[$identifier][$field] <= 0) {
          form_set_error($identifier . '][' . $field, t('Please, provide a value greater then zero.'));
        }
        elseif ($key && $values[$identifier]['min'] >= $values[$identifier]['max']) {
          form_set_error($identifier . '][min', t('Please, make sure that the first value is greater then the second one.'));
        }
      }
    }

    // Validate location
    if ($this->options['location_as'] == 'location' && !empty($values[$identifier][$location])) {
      if ($response = openlayers_proximity_geocode($values[$identifier][$location])) {
        $this->response = $response;
      }
      else {
        form_set_error($identifier . '][' . $location, t('Location not found'));
      }
    }

    // @todo: validate entity
    // @todo: validate argument
  }

  /**
   * Distance from an entity: rough calculation of a centroid for the entity in
   * argument: centroid is calculate directly on values stored in the
   * proximity_index table.
   */
  function process_entity_proximity() {
    $this->value['lat'] = $this->value['lon'] = NULL;
    $this->process_proximity_by_id($this->value['entity']);
  }

  /**
   * Distance from a location: query is a string and it will be geolocated.
   */
  function process_location_proximity() {
    $this->value['lat'] = $this->value['lon'] = NULL;
    if ($this->response = openlayers_proximity_geocode($this->value['location'])) {
      $this->value['lat']  = $this->response[0]['location']['lat'];
      $this->value['lon']  = $this->response[0]['location']['lng'];
    }
    drupal_alter('process_location_proximity', $this->value);
  }

  /**
   * Distance from an argument in the url
   */
  function process_argument_proximity() {
    $this->value['lat'] = $this->value['lon'] = NULL;
    if (isset($this->view->args[$this->value['argument']]))
    {
        $id = $this->view->args[$this->value['argument']];
        if ((string)$id == (string)(int)$id)
        {
            $id = $this->view->args[$this->value['argument']];
            $this->process_proximity_by_id($id);
        }
    }
  }

  function process_proximity_by_id($id) {

    // Get square boundaries.
    $x1 = db_query("SELECT lon FROM {openlayers_proximity} WHERE id = :id AND entity = :entity ORDER BY lon ASC LIMIT 0, 1", array(':id' => $id, ':entity' => $this->entityName))->fetchField();
    $y1 = db_query("SELECT lat FROM {openlayers_proximity} WHERE id = :id AND entity = :entity ORDER BY lat ASC LIMIT 0, 1", array(':id' => $id, ':entity' => $this->entityName))->fetchField();
    $x2 = db_query("SELECT lon FROM {openlayers_proximity} WHERE id = :id AND entity = :entity ORDER BY lon DESC LIMIT 0, 1", array(':id' => $id, ':entity' => $this->entityName))->fetchField();
    $y2 = db_query("SELECT lat FROM {openlayers_proximity} WHERE id = :id AND entity = :entity ORDER BY lat DESC LIMIT 0, 1", array(':id' => $id, ':entity' => $this->entityName))->fetchField();

    $delta = (sqrt(pow($x1 - $x2, 2) + pow($y1 - $y2, 2)) / 2) * OPENLAYERS_PROXIMITY_KM_PER_LAT;
    $this->value['value'] += $delta;
    $this->value['min'] += $delta;
    $this->value['max'] += $delta;
    $this->value['lon'] = ($x1 + $x2) / 2;
    $this->value['lat'] = ($y1 + $y2) / 2;
    drupal_alter('process_entity_proximity', $this->value);
  }

  function secure_input() {
    $this->value['value'] = isset($this->value['value']) ? $this->value['value'] : $this->options['value']['value'];
    $this->value['min'] = isset($this->value['min']) ? $this->value['min'] : $this->options['value']['min'];
    $this->value['max'] = isset($this->value['max']) ? $this->value['max'] : $this->options['value']['max'];
    $this->value['entity'] = isset($this->value[$this->options['expose']['entity']]) ? $this->value[$this->options['expose']['entity']] : $this->options['value']['entity'];
    $this->value['location'] = isset($this->value[$this->options['expose']['location']]) ? $this->value[$this->options['expose']['location']] : $this->options['value']['location'];
    $this->value['argument'] = isset($this->value[$this->options['expose']['argument']]) ? $this->value[$this->options['expose']['argument']] : $this->options['value']['argument'];
    $this->value['unit'] = isset($this->value[$this->options['expose']['unit']]) ? $this->value[$this->options['expose']['unit']] : $this->options['value']['unit'];

    $this->value['value'] = openlayers_proximity_measurement_units_convert($this->value['unit'], $this->value['value']);
    $this->value['min'] = openlayers_proximity_measurement_units_convert($this->value['unit'], $this->value['min']);
    $this->value['max'] = openlayers_proximity_measurement_units_convert($this->value['unit'], $this->value['max']);
  }

  function unset_attributes(&$item, $attributes) {
    foreach ($attributes as $name) {
      unset($item[$name]);
    }
  }

  function clean_exposed_filter_item(&$item) {
    $this->unset_attributes($item, array('#prefix', '#suffix', '#title', '#description'));
  }

  function expose_option_form(&$form, $name, $title) {
    $form['expose']['use_' . $name] = array(
      '#type' => 'checkbox',
      '#title' => t('Unlock @title', array('@title' => $title)),
      '#default_value' => $this->options['expose']['use_' . $name],
      '#description' => t('When checked, this filter will be exposed to the user'),
    );
    $form['expose'][$name] = array(
      '#type' => 'textfield',
      '#default_value' => $this->options['expose'][$name],
      '#title' => t('@title identifier', array('@title' => $title)),
      '#size' => 40,
      '#description' => t('This will appear in the URL after the ? to identify this filter.'),
/*
      '#process' => array(' ctools_dependent_process'),
*/
      '#dependency' => array(
        'edit-options-expose-use-' . $name => array(1),
      ),
    );
  }


  function rename_exposed_filter_item(&$form, $name) {
    $value = $this->options['expose'][$name];
    $identifier = $this->options['expose']['identifier'];
    if ($value != $name) {
      $form[$identifier][$value] = $form[$identifier][$name];
      $this->unset_attributes($form[$identifier], array($name));
    }
    return $value;
  }

  function filter_item_is_exposed($name) {
    return !empty($this->options['expose']['use_' . $name]) && !empty($this->options['expose'][$name]);
  }

  function get_entity_options() {
    $options = array();
    // Avoid query if filter is not exposed. @TODO: get a better condition check.
    if ($this->filter_item_is_exposed('entity') && $this->options['location_as'] == 'entity' || arg(2) == 'views') {
      $base_table = $this->entityInfo['base table'];
      $base_field = $this->entityInfo['entity keys']['id'];
      $base_label = isset($this->entityInfo['entity keys']['label']) ? $this->entityInfo['entity keys']['label'] : '';
      $loadEntity = $this->entityInfo['load hook'];
      $labelEntity = $this->entityInfo['label callback'];
      if (!empty($base_label))
      {
          $results = db_query("SELECT n.$base_label, n." . $base_field . " FROM {openlayers_proximity} op LEFT JOIN {" . $base_table . "} n ON n." . $base_field . " = op.id WHERE n.status = :nstatus AND op.entity = :entity ORDER BY n.$base_label ASC", array(':nstatus' => 1, ':entity' => $this->entityName))->fetchAllAssoc($base_field);
      }
      else
      {
          $results = db_query("SELECT n." . $base_field . " FROM {openlayers_proximity} op LEFT JOIN {" . $base_table . "} n ON n." . $base_field . " = op.id WHERE n.status = :nstatus AND op.entity = :entity ORDER BY n.$base_field ASC", array(':nstatus' => 1, ':entity' => $this->entityName))->fetchAllAssoc($base_field);
      }
      foreach ($results as $row) {
        if (!empty($base_label))
        {
            $options[$row->{$base_field}] = check_plain(drupal_substr($row->{$base_label}, 0, 40));
        }
        else
        {
            $entityObject = $loadEntity($row->{$base_field});
            $options[$row->{$base_field}] = check_plain(drupal_substr($labelEntity($entityObject), 0, 40));
            unset($entityObject);
        }
      }
    }
    return $options;
  }

  function get_argument_options()
  {
      $options = array();
      $path = $this->view->display[$this->view->current_display]->handler->options['path'];
      $argCount = substr_count($path, '%');
      for ($i = 0; $i < $argCount; $i++)
      {
          $options[$i] = t('Argument @id', array('@id' => $i));
      }
      return $options;
  }

  function location_name() {
    $return = '';
    if ($this->options['location_as'] == 'location') {
        $return = $this->options['value']['location'];
    }
    if ($this->options['location_as'] == 'entity') {
        $base_table = $this->entityInfo['base table'];
        $base_field = $this->entityInfo['entity keys']['id'];
        $base_label = isset($this->entityInfo['entity keys']['label']) ? $this->entityInfo['entity keys']['label'] : '';
        $loadEntity = $this->entityInfo['load hook'];
        $labelEntity = $this->entityInfo['label callback'];
        if (!empty($base_label))
        {
            $return = db_query("SELECT $base_label FROM {" . $base_table . "} WHERE " . $base_field . " = :id", array(':id' => $this->options['value']['entity']))->fetchField();
        }
        else
        {
            $entityObject = $loadEntity($this->options['value']['entity']);
            $return = check_plain(drupal_substr($labelEntity($entityObject), 0, 40));
            unset($entityObject);
    }
    }
    if ($this->options['location_as'] == 'argument') {
        // @todo: use better location_name()
        $return = 'argument';
    }
    return $return;
  }

  function get_suffix() {
    $suffix = '';
    $units = openlayers_proximity_get_available_units();
    // If unit is not exposed add unit string to suffix.
    if (!$this->filter_item_is_exposed('unit')) {
      $suffix = '@unit ' . $suffix;
    }

    // If location is not exposed add location string to suffix.
    if (!$this->filter_item_is_exposed($this->options['location_as'])) {
      $suffix = $suffix . ' from @location';
    }
    return t($suffix, array('@unit' => $units[$this->options['value']['unit']]['long'], '@location' => $this->location_name()));
  }


}
