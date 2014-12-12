<module>
	<service><?=$GETCFG_SVC?></service>
	<acl>
		<dos>
<?			echo dump(3, "/acl/dos");
?>		</dos>
		<spi>
<?			echo dump(3, "/acl/spi");
?>		</spi>
		<applications>
<?			echo dump(3, "/acl/applications");
?>		</applications>
		<alg>
<?			echo dump(3, "/device/passthrough");
?>		</alg>
		<cone>
<?			echo dump(3, "/acl/cone");
?>		</cone>
		<anti_spoof>
<?			echo dump(3, "/acl/anti_spoof");
?>		</anti_spoof>
	</acl>
</module>
