$(function() {
	function launch() {
		 $('#popup_form').lightbox_me({centered: true, onLoad: function() { $('#popup_form').find('input:first').focus()}});
	}
	
	$('#btn_addUser').click(function() {
		$("#loader").lightbox_me({centered: true});
		setTimeout(launch, 50);
		return false;
	});
	
	$('table tr:nth-child(even)').addClass('stripe');
});

$(function() {
	function launch() {
		 $('#popup_info').lightbox_me({centered: true, onLoad: function() { $('#popup_info').find('input:first').focus()}});
	}
	
	$('#btn_info').click(function() {
		$("#loader").lightbox_me({centered: true});
		setTimeout(launch, 50);
		return false;
	});
	
	$('table tr:nth-child(even)').addClass('stripe');
});

$(function() {
	function launch() {
		 $('#popup_router').lightbox_me({centered: true, onLoad: function() { $('#popup_router').find('input:first').focus()}});
	}
	
	$('#btn_router').click(function() {
		$("#loader").lightbox_me({centered: true});
		setTimeout(launch, 50);
		return false;
	});
	
	$('table tr:nth-child(even)').addClass('stripe');
});
