<?php

/**
 * Menu callback handler for the object detail page (actor, event, production).
 */
function cnapi_ui_page_detail($object) {
  return array(
    '#theme' => 'cnapi_ui_' . $object['type'],
    '#' . $object['type'] => $object,
  );
}

/**
 * Menu callback handler for the list (context) pages (actor, event, production).
 */
function cnapi_ui_page_list() {

  // Getting the current request as a Drupal request object.
  $request_dp = cnapi_ui_get_active_request();

  $breadcrumb = array();
  $breadcrumb[] = l(t('Home'), '<front>');

  $breadcrumb_array = array();

  if (isset($request_dp['query']['heading'])) {

    // Check if current heading has parent
    $parent = db_query("SELECT hid, name, pid FROM {cnapi_heading} ch WHERE ch.pid <> 0 AND ch.pid NOT IN (SELECT hid FROM {cnapi_heading} ch WHERE ch.pid = 0) AND ch.hid = :hid", array(':hid' => $request_dp['query']['heading']))->fetchObject();
    if ($parent) {
      $parent_heading = db_query("SELECT hid, name FROM {cnapi_heading} ch WHERE ch.hid = :hid", array(':hid' => $parent->pid))->fetchObject();
      $breadcrumb_array[] = array(
        'title' => $parent_heading->name,
        'url' => l($parent_heading->name, 'agenda/search', array('query' => array('heading' => $parent_heading->hid))),
      );
    }

    // Set current heading
    $heading = db_query("SELECT hid, name FROM {cnapi_heading} ch WHERE ch.hid = :hid", array(':hid' => $request_dp['query']['heading']))->fetchObject();
    $breadcrumb_array[] = array(
      'title' => $heading->name,
      'url' => l($heading->name, 'agenda/search', array('query' => array('heading' => $heading->hid))),
    );
  }

  // create heading array to merge if there is no active heading
  $heading_array = array();
  if (isset($heading)) {
    $heading_array = array('heading' => $heading->hid);
  }

  // Set current regio
  if (isset($_GET['regio']) && is_numeric($_GET['regio'])) {
    $regio  = db_query("SELECT name FROM {cnapi_location} WHERE lid = :lid", array(':lid' => $_GET['regio']))->fetchField();
    if ($regio) {
      $breadcrumb_array[] = array(
        'title' => $regio,
        'url' => l($regio, 'agenda/search', array('query' => array_merge($heading_array, array('regio' => $_GET['regio'])))),
      );
    }
  }

  // Set current city
  if (isset($_GET['cityid'])) {

    $regio = db_query("SELECT clh.pid, cl.name FROM {cnapi_location_hierarchy} clh LEFT JOIN {cnapi_location} cl ON cl.lid = clh.pid WHERE clh.lid = :lid AND clh.pid IN (SELECT lid FROM {cnapi_location} WHERE did = :did)", array(':lid' => $_GET['cityid'], ':did' => CNAPI_LOCATIONS_DIMENSION_MUNICIPALITY))->fetchObject();
    if ($regio) {
      $breadcrumb_array[] = array(
        'title' => $regio->name,
        'url' => l($regio->name, 'agenda/search', array('query' => array_merge($heading_array, array('regio' => $regio->pid)))),
      );
    }
    $city  = db_query("SELECT name FROM {cnapi_location} WHERE lid = :lid", array(':lid' => $_GET['cityid']))->fetchField();
    if ($city) {

      // remove ' (region)' from city
      $city_name = explode(' (', $city);
      $city = $city_name[0];

      $city = $regio && $city == $regio->name ? $city . ' centrum' : $city;
      $breadcrumb_array[] = array(
        'title' => $city,
        'url' => l($city, 'agenda/search', array('query' => array_merge($heading_array, array('cityid' => $_GET['cityid'])))),
      );
    }
  }

  // Pop last element to set as page title instead of link
  $breadcrumb_title = array_pop($breadcrumb_array);
  foreach($breadcrumb_array as $value) {
    $breadcrumb[] = $value['url'];
  }

  $breadcrumb[] = $breadcrumb_title['title'];




  // Set breadcrumbs when there is no city or regian facet
//   if (!isset($_GET['regio']) && !isset($_GET['cityid'])) {
//     if (isset($request_dp['query']['heading'])) {
//       $heading = db_query("SELECT hid, name, pid FROM {cnapi_heading} ch WHERE ch.pid <> 0 AND ch.pid NOT IN (SELECT hid FROM {cnapi_heading} ch WHERE ch.pid = 0) AND ch.hid = :hid", array(':hid' => $request_dp['query']['heading']))->fetchAllAssoc('hid');
//       if ($heading) {
//         $parent = db_query("SELECT hid, name FROM {cnapi_heading} ch WHERE ch.hid = :hid", array(':hid' => reset($heading)->pid))->fetchAllAssoc('hid');
//         $parent_url = cnapi_url_dp2dua(array('context' => 'event', 'query' => array('heading' => reset($parent)->hid)));
//         $breadcrumb[] = l(reset($parent)->name, $parent_url['path'], array('query' => array('heading' => reset($parent)->hid)));
//         $breadcrumb[] = reset($heading)->name;
//       }
//       else {
//         $parent = db_query("SELECT hid, name FROM {cnapi_heading} ch WHERE ch.hid = :hid", array(':hid' => $request_dp['query']['heading']))->fetchAllAssoc('hid');
//         $breadcrumb[] = reset($parent)->name;
//       }
//     }
//   }

  drupal_set_breadcrumb($breadcrumb);



  // There's a specific issue with requests with headings that are invalid.
  if (isset($request_dp['query']['heading'])) {
    $headings = $request_dp['query']['heading'];
    $headings = explode(';', $headings);

    foreach ($headings as $heading) {
      if (empty($heading) || !is_numeric($heading)) {
        drupal_not_found();
        exit();
      }
    }
  }

  // Converting it to an API request.
  $request = cnapi_url_dp2p($request_dp);

  // Doing the API request.
  $result = cnapi_get($request);

  // Fetch the total from the result and default to 0.
  $total = isset($result['total']) ? $result['total'] : 0;

  // Fetch the data from the result.
  $data = isset($result['data']) ? $result['data'] : array();

  // Add the API defaults to get the pagelength.
  cnapi_request_add_defaults($request);
  $pagelength = $request['query']['pagelength'];

  // Initialize the Drupal pager.
  pager_default_initialize($total, $pagelength);

  // Get the full context info for the current request.
  $context = cnapi_ui_contexts('id', $request_dp['context']);

  // Add the RSS feed url to the <head>.
  drupal_add_feed(cnapi_ui_rss_feed_url(), cnapi_ui_page_list_title());

  // Return the result as a render array using theme_cnapi_ui_list.
  return array(
    '#theme' => 'cnapi_ui_list',
    '#type' => $context['type'],
    '#items' => $data,
    '#total' => $total,
    '#request' => $request_dp
  );
}

/**
 * Menu callback handler for the RSS feeds.
 */
function cnapi_ui_page_list_rss() {
  // Get the path for the current request. We get this by looking at the Drupal path ($_GET['q']) and stripping of the 'rss/' prefix.
  $path = substr($_GET['q'], strlen('rss/'), strlen($_GET['q']));

  // The $query for the current request is the whole $_GET array without $_GET['q'] (the path).
  $query = $_GET;
  unset($query['q']);

  // Clean invalid query parameters that the API won't understand (or that we don't allow) like utm_source, ...
  cnapi_ui_clean_invalid_queries($query);

  // Clean query paramters that don't matter for RSS.
  cnapi_ui_clean_rss_queries($query);

  // Set the pagelength for the API call to the default feed length.
  $query['pagelength'] = isset($_GET['rss_pagelength']) ? $_GET['rss_pagelength'] : variable_get('feed_default_items', 10);

  // Set the sort field and direction to "created DESC".
  $query['sort'] = 'created DESC';

  // Construct the full path...
  $url = array(
    'path' => $path,
    'options' => array(
      'query' => $query,
    ),
  );

  // ... and convert it to an API request ...
  $request = cnapi_url_dua2p($url);

  // ... and also convert it to a Drupal request.
  $request_dp = cnapi_url_dua2dp($url);

  // Do the API call.
  $result = cnapi_get($request);

  // Fetch the total from the result and default to 0.
  $total = isset($result['total']) ? $result['total'] : 0;

  // Fetch the data from the result.
  $data = isset($result['data']) ? $result['data'] : array();

  // Get the full context info for the current request.
  $context = cnapi_ui_contexts('id', $request_dp['context']);

  // Add the RSS content-type header.
  drupal_add_http_header('Content-Type', 'application/rss+xml; charset=utf-8');

  // Return the themed feed as a render array.
  print theme('cnapi_ui_page_list_rss', array('objects' => $data, 'type' => $context['type']));
}