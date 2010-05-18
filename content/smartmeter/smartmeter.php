<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01//EN" "http://www.w3.org/TR/html4/strict.dtd">
<html>
	<head>
		<meta http-equiv="Content-Type" content="text/html; charset=iso-8859-1" />
		<?php
			include("../../include/smartmeter.inc");
			$uuid=init(empty($_GET["uuid"]) ? '12345678-1234-1234-1234-123456789ABC' : $_GET["uuid"]);
			# only set refresh if max is not set - doesn't make sense if not looking at current second
			if ($max == "") echo "<meta http-equiv='refresh' content='".get_refresh_interval()."' />";
			// das hier ist noch hart verdrahtet - das muss gg. die "dynamische" Variante ersetzt werden: uuid -> DB -> ids
			fill_data_arrays(0, 1, 2);
		?>
		<!-- <link rel="stylesheet" href="style.css" type="text/css" /> -->

		<!--[if IE]><script language="javascript" type="text/javascript" src="../excanvas.min.js"></script><![endif]-->
		<script type="text/javascript" src="../js/jquery.js"></script>
		<script type="text/javascript" src="../flot/jquery.flot.js"></script>
		<meta name="viewport" content="width=<?php screenwidth(); ?>"/>
	</head>
	<body>
      <h3>Leistung [Watt] (von <?php
          echo (date("j.n.Y, G:i", $imp_min));
        ?> bis <?php
          echo (date("j.n.Y, G:i", $imp_max)); 
				?>, zeitliche Aufl&ouml;sung: <?php
					echo ($delta/$teiler);
				?> s):
			<a href="<?php echo $_SERVER['PHP_SELF']; ?>">(Reset)</a>
		</h3>
    <div id="placeholder" style=" <?php width(); ?>"> </div>

		<table width="98%">
			<tr>
				<td width="1%", align="left">
				</td>
				<td width="27%", align="left">
					<a href="<?php move_one_third_left(); ?>">&lt;</a>
				</td>
				<td width="35%", align="center">
					<a href="<?php half_window_size(); ?>">Zeit halbieren</a>
				</td>
				<td width="27%", align="right">
					<a href="<?php move_one_third_right(); ?>">&gt;</a>
				</td>
				<td width="1%", align="left">
				</td>
			</tr>
			<tr>
				<td>
				</td>
				<td align="left">
					<a href="<?php move_left(); ?>">&lt;&lt;</a>
				</td>
				<td align="center">
					<a href="<?php double_window_size(); ?>">Zeit verdoppeln</a>
				</td>
				<td align="right">
					<a href="<?php move_right(); ?>">&gt;&gt;</a>
				</td>
				<td>
				</td>
			</tr>
		</table>
	
		<p id="choices">Anzeigen:<br></p>

		<script id="source" language="javascript" type="text/javascript">
			$(function () {
				var datasets = { <?php /*
					"l1": {
						label: "EG - L1 (P(min): <?php echo round($leistung_PC0_min)?> W, P(max): <?php echo round($leistung_PC0_max) ?> W, P(avg): <?php echo round(calc_leistung_avg("0"),0); ?> W, E: <?php echo round(calc_consumed_energy("0"), 3); ?> kWh, Verbrauch/a: <?php echo round(calc_leistung_avg("0")*24*365/1000,0); ?> kWh)",
						data: [ <?php if ( count ($PC0) > 0 ) { array_walk($PC0, 'array_print'); } ?> ] ,
						color: "blue"
					},*/?>
					"l2": {
						label: "Kapsel (P(min): <?php echo round($leistung_PC1_min)?> W, P(max): <?php echo round($leistung_PC1_max) ?> W, P(avg): <?php echo round(calc_leistung_avg("1"),0); ?> W, E: <?php echo round(calc_consumed_energy("1"), 3); ?> kWh, Verbrauch/a: <?php echo round(calc_leistung_avg("1")*24*365/1000,0); ?> kWh)",
						data: [ <?php if ( count ($PC1) > 0 ) { array_walk($PC1, 'array_print'); } ?> ] ,
						color: "orange"
						} <?php /*,
					"l3": {
						label: "EG - L3 (P(min): <?php echo round($leistung_PC2_min)?> W, P(max): <?php echo round($leistung_PC2_max) ?> W, P(avg): <?php echo round(calc_leistung_avg("2"),0); ?> W, E: <?php echo round(calc_consumed_energy("2"), 3); ?> kWh, Verbrauch/a: <?php echo round(calc_leistung_avg("2")*24*365/1000,0); ?> kWh)",
						data: [ <?php if ( count ($PC2) > 0 ) { array_walk($PC2, 'array_print'); } ?> ] ,
						color: "red"
					},
					"l4": {
						label: "Heizung - (P(min): <?php echo round($leistung_PC3_min)?> W, P(max): <?php echo round($leistung_PC3_max) ?> W, P(avg): <?php echo round(calc_leistung_avg("3"),0); ?> W, E: <?php echo round(calc_consumed_energy("3"), 3); ?> kWh, Verbrauch/a: <?php echo round(calc_leistung_avg("3")*24*365/1000,0); ?> kWh)",
						data: [ <?php if ( count ($PC3) > 0 ) { array_walk($PC3, 'array_print'); } ?> ] ,
						color: "green"
					}*/?>
				};

				// insert checkboxes 
   			var choiceContainer = $("#choices");
   			$.each(datasets, function(key, val) {
       		choiceContainer.append('<input type="checkbox" name="' + key +
                              	'" checked="checked" id="id' + key + '">' +
                              	'<label for="id' + key + '">'
                               	+ val.label + '</label><br>');
   			});
   			choiceContainer.find("input").click(plotAccordingToChoices);
   			function plotAccordingToChoices() {
       		var data = [];
	
       		choiceContainer.find("input:checked").each(function () {
         		var key = $(this).attr("name");
         		if (key && datasets[key])
         		data.push(datasets[key]);
       		});
       		if (data.length > 0)
         	$.plot($("#placeholder"), data, {
          	yaxis: { min: 0 },
           	xaxis: { mode: "time" }
					});
   			}
				plotAccordingToChoices();
			});

		</script>
	</body>
</html>
