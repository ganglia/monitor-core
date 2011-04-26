// Requires the variables 'default_time', 'metric', and 'base_url' already be set.
$(document).ready(function () {
		var options = {
				xaxis: { mode: "time"},
				lines: { show: true },
				points: { show: false },
				grid: { hoverable: true },
		};

		// Iterate over every graph available and plot it
		$(".flotgraph").each(function () {
				var placeholder = $(this);
				var time = placeholder.attr('id').replace('placeholder_', '');
				var url = base_url.replace(default_time, time);

				function onDataReceived(series) {
						series[0].label += " last " + time;
						$.plot(placeholder, series, options);
				 }

				$.ajax({
						url: url,
						method: 'GET',
						dataType: 'json',
						success: onDataReceived
				});

		});
});

