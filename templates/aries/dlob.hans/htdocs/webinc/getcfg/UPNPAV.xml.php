<module>
	<service><?=$GETCFG_SVC?></service>
	<upnpav>		
		<?echo dump(3, "/upnpav");?>		
	</upnpav>
	<runtime>
		<device>
			<storage>
				<count><?echo query("/runtime/device/storage/count");?></count>
			</storage>
		</device>
		<scan_media>
			<?echo dump(2, "/runtime/scan_media");?>
		</scan_media>
	</runtime>		
</module>
