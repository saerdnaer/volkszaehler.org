<?php
	include ('smartmeter.conf');
	function UnixTime($psql_timestamp){
		// WTF?
		if(preg_match('/(\d{4})\-(\d{2})\-(\d{2}) (\d{2}):(\d{2}):(\d{2}).(\d{6})/', $psql_timestamp, $pieces)
			|| preg_match('/(\d{4})\-(\d{2})\-(\d{2}) (\d{2}):(\d{2}):(\d{2}).(\d{5})/', $psql_timestamp, $pieces)
			|| preg_match('/(\d{4})\-(\d{2})\-(\d{2}) (\d{2}):(\d{2}):(\d{2}).(\d{4})/', $psql_timestamp, $pieces)
			|| preg_match('/(\d{4})\-(\d{2})\-(\d{2}) (\d{2}):(\d{2}):(\d{2}).(\d{3})/', $psql_timestamp, $pieces)
			|| preg_match('/(\d{4})\-(\d{2})\-(\d{2}) (\d{2}):(\d{2}):(\d{2}).(\d{2})/', $psql_timestamp, $pieces)
			|| preg_match('/(\d{4})\-(\d{2})\-(\d{2}) (\d{2}):(\d{2}):(\d{2}).(\d{1})/', $psql_timestamp, $pieces)
			|| preg_match('/(\d{4})\-(\d{2})\-(\d{2}) (\d{2}):(\d{2}):(\d{2})/', $psql_timestamp, $pieces)
			|| preg_match('/(\d{2})\-(\d{2})\-(\d{2}) (\d{2}):(\d{2}):(\d{2}).(\d{6})/', $psql_timestamp, $pieces)
			|| preg_match('/(\d{2})\-(\d{2})\-(\d{2}) (\d{2}):(\d{2}):(\d{2}).(\d{5})/', $psql_timestamp, $pieces)
			|| preg_match('/(\d{2})\-(\d{2})\-(\d{2}) (\d{2}):(\d{2}):(\d{2}).(\d{4})/', $psql_timestamp, $pieces)
			|| preg_match('/(\d{4})\-(\d{2})\-(\d{2})/', $psql_timestamp, $pieces)
			|| preg_match('/(\d{2})\-(\d{2})\-(\d{2})/', $psql_timestamp, $pieces)) {
			$unix_time = strtotime($psql_timestamp);
			$unix_time = $unix_time + $pieces[7]/(pow(10, strlen($pieces[7])))." ";
		}
		return $unix_time;
	} 

	function init_psql(){
		global $conn, $smreader_user, $smreader_password;
		$conn = pg_connect("dbname=smartmeter host=localhost user=".$smreader_user." password=".$smreader_password);
		if (!$conn) {
			// couldn't connect
			echo "could not connect localhost";
			exit;
		}
	}
	
	function init($uuid){
		global $interne_id, $conn, $resolution, $min, $imp_min, $max, $imp_max;
		//error_reporting (E_ERROR | E_WARNING | E_PARSE | E_NOTICE);

		if ($uuid == "") {
			echo "Parameter uuid is required - aborting!";
			exit;
		}

		init_psql();
		# parse query string for bad chars
		if(!preg_match('/^([0-9a-zA-Z=\-:.,&%]*)$/',$_SERVER['QUERY_STRING']))
			die("");

		# parse uuid for correct format
		if(!preg_match('/^[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12}$/', $uuid)) {
			echo "uuid has an invalid format - aborting!"; 
			exit;
		}
		
		$result = pg_query($conn, "SELECT id from public.channels where uuid = '$uuid'");
		$line = pg_fetch_array($result);
		// die DB-interne ID des Controllers steht nun in $line['id']
		$interne_id = $line['id'];

		# set (explicit) min and imp_min
		if ( $_GET["min"] != "" ) { 
			$min = UnixTime($_GET["min"]);
			$imp_min = $min;
		} else {
			$imp_min = (date ("U",time())-86400);
		}
		# do the same for max and imp_max
		if ( $_GET["max"] != "" ) { 
			$max = UnixTime($_GET["max"]);
			$imp_max = $max;
		} else {
			$imp_max = date ("U");
		}
		# mal sehen, ob resolution mit angegeben wurde
		if ( $_GET["resolution"] != "" ) {
			$resolution = $_GET["resolution"];
		}
	
		global $teiler, $anfang, $ende, $delta;
		$anfang=date("Y-m-d H:i:s", $imp_min);
		$ende=date("Y-m-d H:i:s", $imp_max);
		$delta=UnixTime($ende)-UnixTime($anfang);

		return $uuid;
	}
	
	function screenwidth(){
		if ( strstr ($_SERVER['HTTP_USER_AGENT'], 'iPhone')) {
			echo '320';
		} else {
			echo '1000';
		} 
	}
	
	function width(){
		if ( strstr ($_SERVER['HTTP_USER_AGENT'], 'iPhone')) {
			echo 'width:'.'310px;height:200px;';
		} else {
			echo 'width:'.'1000px;height:550px;';
		}
	}

	function get_refresh_interval() {
		global $delta;
		# minimum: 2sec.
		$refresh_interval = (round(($delta/500),0)+2);
		return $refresh_interval;
	}

	function current_power($channel) {
	}

	function calc_energy($conn, $id, $anfang, $ende) {
		$result = pg_query($conn, "SELECT COUNT(*) from public.pulses where id='$id' and servertime > '$anfang' and servertime < '$ende'");
		if (!$result) {
			echo "Ein Fehler ist aufgetreten.\n";
			exit;
		}
		$line = pg_fetch_array($result);
		foreach ($line as $col_value) {
			$anzahl = $col_value;
		}
		if ($id == "3") {
			$energy = $anzahl/1000;
		} else {
			$energy = $anzahl/2000;
		}
		return $energy;
	}
	
	function calc_consumed_energy($id) {
		global $conn, $anfang, $ende;

		$energy = calc_energy($conn, $id, $anfang, $ende);
		return $energy;
	}

	function calc_leistung_avg($id) {
		global $conn, $anfang, $ende, $delta;

		$energy = calc_consumed_energy($id);
		$leistung_avg = $energy*1000*3600/($delta);
		return $leistung_avg;
	}

	function fill_array ($conn, $id, $anfang, $ende, $schwelle, $teiler) {
		for ($i=1; $i<$teiler; $i++ ) {
			$delta=UnixTime($ende)-UnixTime($anfang);
			$schritt_anfang = date("Y-m-d H:i:s", (UnixTime($anfang) + $i*$delta/$teiler));
			$schritt_ende = date("Y-m-d H:i:s", UnixTime($anfang) + ($i+1)*$delta/$teiler);
			$schritt_delta = UnixTime($schritt_ende)-UnixTime($schritt_anfang);
			$result = pg_query($conn, "SELECT COUNT(*) from public.pulses where id='$id' and servertime > '$schritt_anfang' and servertime < '$schritt_ende'");
			$anzahl=0;
			$schritt_delta = UnixTime($schritt_ende)-UnixTime($schritt_anfang);
			if (!$result) {
				echo "Ein Fehler ist aufgetreten.\n";
				exit;
			}
			$line = pg_fetch_row($result);
			foreach ($line as $col_value) {
				$anzahl = $col_value;
			}
			if (($anzahl >= $schwelle ) or ($anzahl = 0)){
				# -> "Zaehlmodus" = es sind genügend Impulse für die Berechnung eines zuverlässigen Durchschnitts für dieses Intervall vorhanden
				if ($id == "3") {
					$leistung = $anzahl/1000*1000*3600/($schritt_delta);
				} else {
					$leistung = $anzahl/2000*1000*3600/($schritt_delta);
				}
				$neue_Zeit = $schritt_anfang;
				$PC [ UnixTime($neue_Zeit) ] = $leistung;
			} else {
				# -> "Direktmodus" = alle Messwerte auslesen
				$result = pg_query($conn, "SELECT * from public.pulses where id='$id' and servertime > '$schritt_anfang' and servertime < '$schritt_ende'");
				while($line = pg_fetch_array($result)) {
					$alte_Zeit = $neue_Zeit;
					$neue_Zeit = $line['servertime'];
					if (($alte_Zeit != 0) and ($neue_Zeit != 0)) {
						# nur wenn beide != 0 sind
						$differenz = UnixTime($neue_Zeit) - UnixTime($alte_Zeit);
						if ($id == "3") {
							$leistung = 3600*1000/($differenz*1000);
						} else {
							$leistung = 3600*1000/($differenz*2000);
						}
						$PC [ UnixTime($neue_Zeit) ] = $leistung;
					}
					$neue_Zeit = $neue_Zeit;
				} 
			}
		}
		return $PC;
	}

	function fill_data_arrays($id_0, $id_1, $id_2, $id_3 = '') {
		global $resolution, $conn, $anfang, $ende, $delta, $teiler, $leistung_PC0, $leistung_PC1, $leistung_PC2, $leistung_PC3, $PC0, $PC1, $PC2, $PC3, $leistung_PC0_max, $leistung_PC1_max, $leistung_PC2_max, $leistung_PC3_max, $leistung_PC0_min, $leistung_PC1_min, $leistung_PC2_min, $leistung_PC3_min;
		$schwelle = 10;
		if ($resolution != "") {
			$teiler=($delta/$resolution);
		} else {
			$teiler=100;
		}
		if (!$id_0 == '') $PC0 = fill_array($conn, $id_0, $anfang, $ende, $schwelle, $teiler);
		if (!$id_1 == '') $PC1 = fill_array($conn, $id_1, $anfang, $ende, $schwelle, $teiler);
		if (!$id_2 == '') $PC2 = fill_array($conn, $id_2, $anfang, $ende, $schwelle, $teiler);
		if (!$id_3 == '') $PC3 = fill_array($conn, $id_3, $anfang, $ende, $schwelle, $teiler);

		if (count($PC0)) { 
			$leistung_PC0_min = min($PC0); 
			$leistung_PC0_max = max($PC0); 
		};
		if (count($PC1)) { 
			$leistung_PC1_min = min($PC1); 
			$leistung_PC1_max = max($PC1); 
		};
		if (count($PC2)) { 
			$leistung_PC2_min = min($PC2); 
			$leistung_PC2_max = max($PC2); 
		};
		if (count($PC3)) { 
			$leistung_PC3_min = min($PC3); 
			$leistung_PC3_max = max($PC3); 
		};
	}

	function array_print($item2, $key) {
		#js erwartet unixtimestamp in us -> *1000
		#js berücksichtigt nicht die Zeitzone -> Korrektur für Arme: +3600s
		# Sommerzeit: *2 --Andi
		$keyx1000 = 1000*($key+3600*2);
		echo "[ $keyx1000, $item2], ";
	}	
		
	function move_one_third_left() {
		global $resolution, $imp_min, $imp_max, $delta, $uuid;
		echo $_SERVER['PHP_SELF']."?min=".date("Y-m-d, H:i:s", $imp_min - $delta/3);
		echo "&max=".date("Y-m-d, H:i:s", $imp_max - $delta/3);
		if ( $resolution != "" ) { 
			echo "&resolution=". $resolution;
		}
		//echo "&uuid=".$uuid;
	}
		
	function move_one_third_right() {
		global $resolution, $imp_min, $imp_max, $delta, $uuid;
		echo $_SERVER['PHP_SELF']."?min=".date("Y-m-d, H:i:s", $imp_min + $delta/3);
		echo "&max=".date("Y-m-d, H:i:s", $imp_max + $delta/3);
		if ( $resolution != "" ) { 
			echo "&resolution=". $resolution;
		}
		//echo "&uuid=".$uuid;
	}
		
	function move_left() {
		global $resolution, $imp_min, $imp_max, $delta, $uuid;
		echo $_SERVER['PHP_SELF']."?min=".date("Y-m-d, H:i:s", $imp_min - $delta);
		echo "&max=".date("Y-m-d, H:i:s", $imp_max - $delta);
		if ( $resolution != "" ) { 
			echo "&resolution=". $resolution;
		}
		//echo "&uuid=".$uuid;
	}
		
	function move_right() {
		global $resolution, $imp_min, $imp_max, $delta, $uuid;
		echo $_SERVER['PHP_SELF']."?min=".date("Y-m-d, H:i:s", $imp_min + $delta);
		echo "&max=".date("Y-m-d, H:i:s", $imp_max + $delta);
		if ( $resolution != "" ) { 
			echo "&resolution=". $resolution;
		}
		//echo "&uuid=".$uuid;
	}
		
	function half_window_size(){
		global $resolution, $imp_min, $imp_max, $max, $uuid;
		echo $_SERVER['PHP_SELF']."?min=".date("Y-m-d, H:i:s", $imp_max - ($imp_max-$imp_min)/2 );
		if ( $max != "" ) { 
			echo "&max=".date("Y-m-d, H:i:s", $imp_max);
		}
		if ( $resolution != "" ) { 
			echo "&resolution=". $resolution;
		}
		//echo "&uuid=".$uuid;
	}

	function double_window_size(){
		global $resolution, $imp_min, $imp_max, $max, $uuid;
		echo $_SERVER['PHP_SELF']."?min=".date("Y-m-d, H:i:s", $imp_max - ($imp_max-$imp_min)*2 );
		if ( $max != "" ) { 
			echo "&max=".date("Y-m-d, H:i:s", $imp_max);
		}
		if ( $resolution != "" ) { 
			echo "&resolution=". $resolution;
		}
		//echo "&uuid=".$uuid;
	}
?>
